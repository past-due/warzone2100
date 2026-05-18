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
 * \brief `LoadingTask` and `ResourceLoadingJob` implementation.
 */

#include "loading_task.h"

#include "lib/framework/wzapp.h"

LoadOutcome LoadingTask::result() const noexcept
{
	return coro ? coro.promise().result : LoadOutcome::Failure;
}

void LoadingTask::NestedAwaiter::await_suspend(std::coroutine_handle<> h)
{
	parent = h;
	auto child_coro = child->coro;
	child->coro = {};

	auto parent_coro = std::coroutine_handle<LoadingTaskPromise>::from_address(h.address());
	ResourceLoadingController *controller = parent_coro.promise().controller;
	ASSERT(controller, "co_await LoadingTask from a coroutine that is not bound to a ResourceLoadingController");

	auto &child_promise = child_coro.promise();
	if (child_promise.controller == nullptr)
	{
		child_promise.controller = controller;
	}

	controller->push_nested(child_coro, parent);
}

ResourceLoadingJob::ResourceLoadingJob(LoadingTask task, FinalizeCallback onSuccessIn, FinalizeCallback onFailureIn)
	: pending_task(std::move(task))
	, onSuccess(std::move(onSuccessIn))
	, onFailure(std::move(onFailureIn))
{
}

ResourceLoadingJob::ResourceLoadingJob(TaskFactory taskFactory,
                                       FinalizeCallback onSuccessIn,
                                       FinalizeCallback onFailureIn,
                                       ResourceLoadingController::FrameProcessingMode initialFrameMode)
	: task_factory(std::move(taskFactory))
	, onSuccess(std::move(onSuccessIn))
	, onFailure(std::move(onFailureIn))
	, initial_frame_mode(initialFrameMode)
	, use_factory(true)
{
}

void ResourceLoadingJob::bindAndStart(ResourceLoadingController &controller)
{
	controller.reset_task_state();
	if (use_factory)
	{
		ASSERT(task_factory, "ResourceLoadingJob factory is null");
		controller.start(task_factory(controller));
	}
	else
	{
		controller.start(std::move(pending_task));
	}
	controller.set_frame_processing_mode(initial_frame_mode);
}

LoadStepStatus ResourceLoadingJob::step(ResourceLoadingController &controller)
{
	return controller.step_one_quantum();
}

void ResourceLoadingJob::finalizeSuccess()
{
	if (onSuccess)
	{
		onSuccess();
	}
}

void ResourceLoadingJob::finalizeFailure()
{
	if (onFailure)
	{
		onFailure();
	}
}


std::unique_ptr<ResourceLoadingJob> makeResourceLoadingJob(
    ResourceLoadingJob::TaskFactory taskFactory,
    ResourceLoadingJob::FinalizeCallback onSuccess,
    ResourceLoadingJob::FinalizeCallback onFailure,
    ResourceLoadingController::FrameProcessingMode initialFrameMode)
{
	return std::make_unique<ResourceLoadingJob>(
	    std::move(taskFactory), std::move(onSuccess), std::move(onFailure), initialFrameMode);
}

bool runLoadingJobToCompletion(std::unique_ptr<ResourceLoadingJob> job)
{
	ResourceLoadingController runner;
	return runner.runJobToCompletion(std::move(job));
}
