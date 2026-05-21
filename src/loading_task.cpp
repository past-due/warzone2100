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

void LoadingTaskPromise::FinalAwaiter::await_suspend(std::coroutine_handle<LoadingTaskPromise> h) const noexcept
{
	auto &promise = h.promise();
	if (promise.controller != nullptr)
	{
		ASSERT(promise.controller->topFrame().handle.address() == h.address(),
		       "completing frame must be execution stack top");
		promise.controller->onFrameFinished(promise.result);
	}
}

LoadOutcome LoadingTask::ChildTaskAwaiter::await_resume() const noexcept
{
	if (child_handle)
	{
		ASSERT(child_handle.done(), "nested child must be done before destroy");
		LoadOutcome const outcome = child_handle.promise().result;
		child_handle.destroy();
		return outcome;
	}
	return child ? child->result() : LoadOutcome::Failure;
}

void LoadingTask::ChildTaskAwaiter::await_suspend(std::coroutine_handle<> h)
{
	ASSERT(child != nullptr, "co_await null LoadingTask");
	auto child_coro = child->coro;
	ASSERT(child_coro, "co_await empty LoadingTask");
	ASSERT(!child_coro.done(), "co_await already-completed LoadingTask");
	child->coro = {};
	child_handle = child_coro;

	auto parent_coro = std::coroutine_handle<LoadingTaskPromise>::from_address(h.address());
	ResourceLoadingController *controller = parent_coro.promise().controller;
	ASSERT(controller, "co_await LoadingTask from a coroutine that is not bound to a ResourceLoadingController");
	ASSERT(controller->topFrame().handle.address() == h.address(),
	       "co_await child must suspend the execution stack top");

	auto &child_promise = child_coro.promise();
	if (child_promise.controller == nullptr)
	{
		child_promise.controller = controller;
	}

	controller->pushFrame(child_coro);
}

ResourceLoadingJob::ResourceLoadingJob(TaskFactory taskFactory,
                                       FinalizeCallback onSuccessIn,
                                       FinalizeCallback onFailureIn,
                                       ResourceLoadingController::FrameProcessingMode initialFrameMode)
	: task_factory(std::move(taskFactory))
	, onSuccess(std::move(onSuccessIn))
	, onFailure(std::move(onFailureIn))
	, initialFrameMode(initialFrameMode)
{
}

void ResourceLoadingJob::bindAndStart(ResourceLoadingController &controller)
{
	controller.resetTaskState();
	ASSERT(task_factory, "ResourceLoadingJob factory is null");
	controller.start(task_factory(controller));
	controller.setFrameProcessingMode(initialFrameMode);
}

LoadStepStatus ResourceLoadingJob::step(ResourceLoadingController &controller)
{
	return controller.stepOneQuantum();
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
