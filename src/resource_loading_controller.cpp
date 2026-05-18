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

void ResourceLoadingController::start(LoadingTask task)
{
	ASSERT(!rootCoro, "ResourceLoadingController.start called while a task is already active");
	rootCoro = task.release();
	ASSERT(rootCoro, "ResourceLoadingController.start given an empty task");
	rootCoro.promise().controller = this;
	taskFinished = false;
	root_outcome = LoadOutcome::Success;
	current = rootCoro;
	parentCoro = {};
	frameMode = FrameProcessingMode::ConsumeFrame;
}

LoadStepStatus ResourceLoadingController::stepOneQuantum()
{
	ASSERT(rootCoro, "ResourceLoadingController.step_one_quantum without an active task");

	if (taskFinished)
	{
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	if (!current)
	{
		current = rootCoro;
	}

	current.resume();

	if (taskFinished)
	{
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	if (rootCoro.done())
	{
		onRootTaskFinished(rootCoro.promise().result);
		return root_outcome == LoadOutcome::Success ? LoadStepStatus::Completed : LoadStepStatus::Failed;
	}

	return LoadStepStatus::InProgress;
}

void ResourceLoadingController::resetTaskState() noexcept
{
	if (rootCoro)
	{
		rootCoro.destroy();
		rootCoro = {};
	}
	current = {};
	parentCoro = {};
	taskFinished = false;
	root_outcome = LoadOutcome::Success;
	frameMode = FrameProcessingMode::ConsumeFrame;
}

void ResourceLoadingController::onRootTaskFinished(LoadOutcome result) noexcept
{
	root_outcome = result;
	taskFinished = true;
	current = {};
	parentCoro = {};
	if (rootCoro)
	{
		rootCoro.destroy();
		rootCoro = {};
	}
}

void ResourceLoadingController::onNestedChildFinished() noexcept
{
	if (current && (!rootCoro || current.address() != rootCoro.address()))
	{
		current.destroy();
	}
	current = parentCoro;
	parentCoro = {};
}

void ResourceLoadingController::pushNested(std::coroutine_handle<LoadingTaskPromise> child,
                                            std::coroutine_handle<> parent) noexcept
{
	parentCoro = parent;
	current = child;
}

bool ResourceLoadingController::runJobToCompletion(std::unique_ptr<ResourceLoadingJob> job)
{
	ASSERT(job, "runJobToCompletion called with null job");
	ASSERT(!rootCoro, "runJobToCompletion called while this controller already has an active task");

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
