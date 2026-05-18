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
/** \file loading_task.cpp
 * \brief `LoadingScheduler` implementation.
 */

#include "loading_task.h"

#include "lib/framework/wzapp.h"

LoadOutcome LoadingTask::result() const noexcept
{
	return coro ? coro.promise().result : LoadOutcome::Failure;
}

LoadingScheduler::FrameYield LoadingScheduler::yield_frame() noexcept
{
	return FrameYield{this};
}

LoadingScheduler::Bind LoadingScheduler::bind() noexcept
{
	return Bind{this};
}

LoadingScheduler::SetFrameMode LoadingScheduler::set_frame_mode(
    ResourceLoadingController::FrameProcessingMode mode) noexcept
{
	return SetFrameMode{this, mode};
}

void LoadingScheduler::start(LoadingTask task)
{
	ASSERT(!root_coro, "LoadingScheduler.start called while a task is already active");
	root_coro = task.release();
	ASSERT(root_coro, "LoadingScheduler.start given an empty task");
	task_finished = false;
	root_outcome = LoadOutcome::Success;
	current = root_coro;
	parent_coro = {};
	frame_mode = ResourceLoadingController::FrameProcessingMode::ConsumeFrame;
}

LoadStepStatus LoadingScheduler::step_one_quantum()
{
	ASSERT(root_coro, "LoadingScheduler.step_one_quantum without an active task");

	if (task_finished)
	{
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	if (!current)
	{
		current = root_coro;
	}

	current.resume();

	if (task_finished)
	{
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	if (root_coro.done())
	{
		on_root_task_finished(root_coro.promise().result);
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	return LoadStepStatus::InProgress;
}

void LoadingScheduler::on_root_task_finished(LoadOutcome result) noexcept
{
	root_outcome = result;
	task_finished = true;
	current = {};
	parent_coro = {};
	if (root_coro)
	{
		root_coro.destroy();
		root_coro = {};
	}
}

void LoadingScheduler::on_nested_child_finished() noexcept
{
	if (current && (!root_coro || current.address() != root_coro.address()))
	{
		current.destroy();
	}
	current = parent_coro;
	parent_coro = {};
}

void LoadingScheduler::push_nested(std::coroutine_handle<LoadingTaskPromise> child,
                                   std::coroutine_handle<> parent) noexcept
{
	parent_coro = parent;
	current = child;
}

void LoadingTask::NestedAwaiter::await_suspend(std::coroutine_handle<> h)
{
	parent = h;
	auto child_coro = child->coro;
	child->coro = {};
	child_coro.promise().scheduler->push_nested(child_coro, parent);
}

namespace
{

IResourceLoadingJob::StepResult to_step_result(LoadStepStatus status)
{
	switch (status)
	{
	case LoadStepStatus::InProgress:
		return IResourceLoadingJob::StepResult::InProgress;
	case LoadStepStatus::Completed:
		return IResourceLoadingJob::StepResult::Completed;
	case LoadStepStatus::Failed:
		return IResourceLoadingJob::StepResult::Failed;
	}
	return IResourceLoadingJob::StepResult::Failed;
}

} // namespace

CoroutineLoadingJob::CoroutineLoadingJob(LoadingTask task, FinalizeCallback onSuccessIn, FinalizeCallback onFailureIn)
	: onSuccess(std::move(onSuccessIn))
	, onFailure(std::move(onFailureIn))
{
	scheduler.start(std::move(task));
}

IResourceLoadingJob::StepResult CoroutineLoadingJob::step()
{
	return to_step_result(scheduler.step_one_quantum());
}

void CoroutineLoadingJob::finalizeSuccess()
{
	if (onSuccess)
	{
		onSuccess();
	}
}

void CoroutineLoadingJob::finalizeFailure()
{
	if (onFailure)
	{
		onFailure();
	}
}

ResourceLoadingController::FrameProcessingMode CoroutineLoadingJob::frameProcessingMode() const
{
	return scheduler.frame_processing_mode();
}

std::unique_ptr<IResourceLoadingJob> makeCoroutineLoadingJob(LoadingTask task,
                                                             CoroutineLoadingJob::FinalizeCallback onSuccess,
                                                             CoroutineLoadingJob::FinalizeCallback onFailure)
{
	return std::make_unique<CoroutineLoadingJob>(std::move(task), std::move(onSuccess), std::move(onFailure));
}

void requestCoroutineLoad(ResourceLoadingController &controller,
                          ResourceLoadingRequest request,
                          LoadingTask task,
                          CoroutineLoadingJob::FinalizeCallback finalizeSuccess,
                          CoroutineLoadingJob::FinalizeCallback finalizeFailure)
{
	controller.request(std::move(request),
	                   makeCoroutineLoadingJob(std::move(task), std::move(finalizeSuccess), std::move(finalizeFailure)));
}
