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

ResourceLoadingController::FrameYield ResourceLoadingController::yield_frame() noexcept
{
	return FrameYield{this};
}

ResourceLoadingController::Bind ResourceLoadingController::bind() noexcept
{
	return Bind{this};
}

ResourceLoadingController::SetFrameMode ResourceLoadingController::set_frame_mode(FrameProcessingMode mode) noexcept
{
	return SetFrameMode{this, mode};
}

void ResourceLoadingController::start(LoadingTask task)
{
	ASSERT(!root_coro, "ResourceLoadingController.start called while a task is already active");
	root_coro = task.release();
	ASSERT(root_coro, "ResourceLoadingController.start given an empty task");
	root_coro.promise().controller = this;
	task_finished = false;
	root_outcome = LoadOutcome::Success;
	current = root_coro;
	parent_coro = {};
	frame_mode = FrameProcessingMode::ConsumeFrame;
}

LoadStepStatus ResourceLoadingController::step_one_quantum()
{
	ASSERT(root_coro, "ResourceLoadingController.step_one_quantum without an active task");

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

void ResourceLoadingController::reset_task_state() noexcept
{
	if (root_coro)
	{
		root_coro.destroy();
		root_coro = {};
	}
	current = {};
	parent_coro = {};
	task_finished = false;
	root_outcome = LoadOutcome::Success;
	frame_mode = FrameProcessingMode::ConsumeFrame;
}

void ResourceLoadingController::on_root_task_finished(LoadOutcome result) noexcept
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

void ResourceLoadingController::on_nested_child_finished() noexcept
{
	if (current && (!root_coro || current.address() != root_coro.address()))
	{
		current.destroy();
	}
	current = parent_coro;
	parent_coro = {};
}

void ResourceLoadingController::push_nested(std::coroutine_handle<LoadingTaskPromise> child,
                                            std::coroutine_handle<> parent) noexcept
{
	parent_coro = parent;
	current = child;
}

bool ResourceLoadingController::runJobToCompletion(std::unique_ptr<ResourceLoadingJob> job)
{
	ASSERT(job, "runJobToCompletion called with null job");
	ASSERT(!root_coro, "runJobToCompletion called while this controller already has an active task");

	job->bindAndStart(*this);

	while (true)
	{
		switch (step_one_quantum())
		{
		case LoadStepStatus::InProgress:
			continue;
		case LoadStepStatus::Completed:
			job->finalizeSuccess();
			reset_task_state();
			return true;
		case LoadStepStatus::Failed:
			job->finalizeFailure();
			reset_task_state();
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

void ResourceLoadingController::request(ResourceLoadingRequest requestIn, std::unique_ptr<ResourceLoadingJob> job)
{
	ASSERT(job, "ResourceLoadingController.request called with null job");
	if (activeJob)
	{
		queuedRequest = std::move(requestIn);
		queuedJob = std::move(job);
		return;
	}

	begin(std::move(requestIn), std::move(job));
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
	reset_task_state();

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
	return frame_processing_mode();
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
