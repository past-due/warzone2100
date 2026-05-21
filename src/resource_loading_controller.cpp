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
#include "init.h"
#include "main_resource_loading.h"
#include "multiint.h"
#include "wrappers.h"

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

ExecutionFrame &ResourceLoadingController::topFrame()
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

ExecutionFrame const &ResourceLoadingController::topFrame() const
{
	ASSERT(hasActiveExecution(), "topFrame without active execution");
	return executionStack.top();
}

void ResourceLoadingController::pushFrame(std::coroutine_handle<> handle)
{
	ASSERT(handle, "pushFrame with null handle");
	ASSERT(!handle.done(), "pushFrame with already-completed coroutine");
	ASSERT(!sessionFinished, "pushFrame after load session finished");
	executionStack.push(ExecutionFrame{handle, ExecutionFrameState::Paused});
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
	// Nested completion: keep `finished` alive until the parent's NestedAwaiter::await_resume.
}

void ResourceLoadingController::start(LoadingTask task)
{
	ASSERT(!hasActiveExecution(), "ResourceLoadingController.start called while execution is active");
	std::coroutine_handle<LoadingTaskPromise> const root = task.release();
	ASSERT(root, "ResourceLoadingController.start given an empty task");
	root.promise().controller = this;
	sessionFinished = false;
	terminalOutcome = LoadOutcome::Success;
	frameMode = FrameProcessingMode::ConsumeFrame;
	pushFrame(root);
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
	frameMode = FrameProcessingMode::ConsumeFrame;
	ASSERT(!hasActiveExecution() && !sessionFinished, "resetTaskState must clear execution state");
}

bool ResourceLoadingController::runJobToCompletion(std::unique_ptr<ResourceLoadingJob> job)
{
	ASSERT(job, "runJobToCompletion called with null job");
	ASSERT(!hasActiveExecution(), "runJobToCompletion called while this controller already has active execution");

	job->bindAndStart(*this);

	while (true)
	{
		switch (stepOneQuantum())
		{
		case LoadStepStatus::InProgress:
			continue;
		case LoadStepStatus::Completed:
			job->finalizeSuccess();
			resetTaskState();
			return true;
		case LoadStepStatus::Failed:
			job->finalizeFailure();
			resetTaskState();
			return false;
		}
	}
}

void ResourceLoadingController::request(ResourceLoadingRequest requestIn)
{
	if (activeJob)
	{
		queuedRequest = std::move(requestIn);
		queuedJob.reset();
		return;
	}

	begin(std::move(requestIn));
}

void ResourceLoadingController::begin(ResourceLoadingRequest requestIn, std::unique_ptr<ResourceLoadingJob> job)
{
	ASSERT(!activeJob, "LoadingController.begin called while another loading job is active");
	activeRequest = std::move(requestIn);
	activeJob = job ? std::move(job) : makeJob(activeRequest.value());
	ASSERT(activeJob, "Failed to create loading job");

	activeJob->bindAndStart(*this);

	const bool hadLoadingScreen = isLoadingScreenActive();
	if (activeRequest->showLoadingScreen && !hadLoadingScreen)
	{
		initLoadingScreen(activeRequest->drawBackdrop);
	}
}

bool ResourceLoadingController::active() const
{
	return activeJob != nullptr;
}

void ResourceLoadingController::step()
{
	ASSERT(activeJob, "LoadingController.step called without an active job");
	LoadStepStatus const result = activeJob->step(*this);
	switch (result)
	{
	case LoadStepStatus::InProgress:
		return;
	case LoadStepStatus::Completed:
		activeJob->finalizeSuccess();
		break;
	case LoadStepStatus::Failed:
		activeJob->finalizeFailure();
		break;
	}

	activeJob.reset();
	activeRequest.reset();
	resetTaskState();

	if (queuedRequest.has_value())
	{
		ResourceLoadingRequest nextRequest = std::move(queuedRequest.value());
		std::unique_ptr<ResourceLoadingJob> nextJob = std::move(queuedJob);
		queuedRequest.reset();
		queuedJob.reset();
		begin(std::move(nextRequest), std::move(nextJob));
	}
}

void ResourceLoadingController::presentLoadingScreenIfNeeded()
{
	if (activeRequest && activeRequest->showLoadingScreen)
	{
		presentLoadingScreenForCurrentFrame();
	}
}

ResourceLoadingController::FrameProcessingMode ResourceLoadingController::currentFrameProcessingMode() const
{
	ASSERT(activeJob, "LoadingController.currentFrameProcessingMode called without an active job");
	return frameProcessingMode();
}

bool ResourceLoadingController::loadingScreenHandledByController() const
{
	return activeJob != nullptr && activeRequest.has_value() && activeRequest->showLoadingScreen;
}

std::unique_ptr<ResourceLoadingJob> ResourceLoadingController::makeJob(const ResourceLoadingRequest &request)
{
	switch (request.kind)
	{
	case ResourceLoadingRequest::Kind::FrontendInit:
		return makeFrontendInitJob(request);
	case ResourceLoadingRequest::Kind::StartGame:
		return main_resource_loading::makeStartGameResourceJob();
	case ResourceLoadingRequest::Kind::LoadSaveGame:
		return main_resource_loading::makeLoadSaveGameResourceJob();
	case ResourceLoadingRequest::Kind::MapPreview:
		return makeMapPreviewJob(request);
	}
	return nullptr;
}
