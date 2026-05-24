// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** \file resource_loading_controller.cpp
 * \brief Implementation of the cooperative loading controller.
 */

#include "resource_loading_controller.h"

#include "loading_task.h"

#include "lib/framework/wzapp.h"

#include <utility>

ResourceLoadingController& ResourceLoadingController::instance()
{
	static ResourceLoadingController instance;
	return instance;
}

ResourceLoadingController::FrameYield ResourceLoadingController::yieldFrame() noexcept
{
	return FrameYield{this};
}

void ResourceLoadingController::FrameYield::await_suspend(std::coroutine_handle<> h) const noexcept
{
	ASSERT(controller != nullptr, "yieldFrame without controller");
	auto &top = controller->topFrame();
	ASSERT(top.handle.address() == h.address(),
	       "yieldFrame must suspend the execution stack top");
	top.state = ExecutionFrameState::Paused;
}

ResourceLoadingController::ExecutionFrame &ResourceLoadingController::topFrame()
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

ResourceLoadingController::ExecutionFrame const &ResourceLoadingController::topFrame() const
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

void ResourceLoadingController::pushFrame(std::coroutine_handle<> handle, FramePolicy policy)
{
	ASSERT(handle, "pushFrame with null handle");
	ASSERT(!handle.done(), "pushFrame with already-completed coroutine");
	ASSERT(!sessionFinished, "pushFrame after load session finished");
	executionStack.push(ExecutionFrame{handle, ExecutionFrameState::Paused, policy});
}

void ResourceLoadingController::popAndDestroyTop() noexcept
{
	if (executionStack.empty())
	{
		return;
	}
	std::coroutine_handle<> const handle = executionStack.top().handle;
	executionStack.pop();
	if (handle)
	{
		handle.destroy();
	}
}

void ResourceLoadingController::onFrameFinished(LoadOutcome outcome) noexcept
{
	ASSERT(!sessionFinished, "onFrameFinished after load session finished");
	ASSERT(hasActiveExecution(), "onFrameFinished without active execution");
	std::coroutine_handle<> const finished = executionStack.top().handle;
	ASSERT(finished.done(), "onFrameFinished for a coroutine that is not done");
	executionStack.pop();
	if (executionStack.empty())
	{
		if (finished)
		{
			finished.destroy();
		}
		terminalOutcome = outcome;
		sessionFinished = true;
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
	}
	// Nested completion: keep `finished` alive until the parent's ChildTaskAwaiter::await_resume.
}

void ResourceLoadingController::start(LoadingTask task, FramePolicy policy)
{
	ASSERT(!hasActiveExecution(), "ResourceLoadingController.start called while execution is active");
	std::coroutine_handle<LoadingTaskPromise> const root = task.release();
	ASSERT(root, "ResourceLoadingController.start given an empty task");
	root.promise().controller = this;
	sessionFinished = false;
	terminalOutcome = LoadOutcome::Success;
	pushFrame(root, policy);
	ASSERT(executionStack.size() == 1, "start must leave a single root execution frame");
}

LoadStepStatus ResourceLoadingController::stepOneQuantum()
{
	if (sessionFinished)
	{
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
		return terminalOutcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	ASSERT(hasActiveExecution(), "ResourceLoadingController.stepOneQuantum without active execution");

	ExecutionFrame &top = topFrame();
	ASSERT(top.state == ExecutionFrameState::Paused,
	       "stepOneQuantum must resume a paused execution frame");
	top.state = ExecutionFrameState::Running;
	top.handle.resume();

	if (sessionFinished)
	{
		ASSERT(!hasActiveExecution(), "sessionFinished requires empty execution stack");
		return terminalOutcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	ASSERT(hasActiveExecution() && !sessionFinished,
	       "InProgress requires active execution and an unfinished session");
	return LoadStepStatus::InProgress;
}

void ResourceLoadingController::resetTaskState() noexcept
{
	while (!executionStack.empty())
	{
		popAndDestroyTop();
	}
	sessionFinished = false;
	terminalOutcome = LoadOutcome::Success;
	ASSERT(!hasActiveExecution() && !sessionFinished, "resetTaskState must clear execution state");
}

void ResourceLoadingController::request(std::unique_ptr<ResourceLoadingJob> job, FramePolicy policy)
{
	ASSERT(job, "request given null job");
	ResourceLoadingSubmission submission;
	submission.job = std::move(job);
	submission.policy = policy;

	if (activeSubmission.has_value())
	{
		queuedSubmission = std::move(submission);
		return;
	}

	begin(std::move(submission));
}

void ResourceLoadingController::begin(ResourceLoadingSubmission submission)
{
	ASSERT(!activeSubmission.has_value(), "begin called while another submission is active");
	ASSERT(submission.job, "begin given null job");
	ResourceLoadingJob *job = submission.job.get();
	FramePolicy const policy = submission.policy;
	activeSubmission = std::move(submission);
	job->bindAndStart(*this, policy);
}

bool ResourceLoadingController::active() const
{
	return activeSubmission.has_value();
}

void ResourceLoadingController::step()
{
	ASSERT(activeSubmission.has_value(), "step called without an active submission");
	LoadStepStatus const result = activeSubmission->job->step(*this);
	if (result == LoadStepStatus::InProgress)
	{
		return;
	}

	activeSubmission.reset();
	resetTaskState();

	if (queuedSubmission.has_value())
	{
		ResourceLoadingSubmission nextSubmission = std::move(queuedSubmission.value());
		queuedSubmission.reset();
		begin(std::move(nextSubmission));
	}
}

ResourceLoadingController::FrameProcessingMode ResourceLoadingController::currentFrameProcessingMode() const
{
	ASSERT(activeSubmission.has_value(), "currentFrameProcessingMode without active submission");
	ASSERT(hasActiveExecution(), "currentFrameProcessingMode without active execution");
	return topFrame().policy.frameMode;
}

bool ResourceLoadingController::loadingScreenHandledByController() const
{
	if (!activeSubmission.has_value())
	{
		return false;
	}
	if (!hasActiveExecution())
	{
		return activeSubmission->policy.showLoadingScreen;
	}
	return topFrame().policy.showLoadingScreen;
}
