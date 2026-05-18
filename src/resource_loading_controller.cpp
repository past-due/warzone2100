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

void ResourceLoadingController::request(ResourceLoadingRequest requestIn,
                                      std::unique_ptr<ResourceLoadingJob> job)
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
	LoadStepStatus const result = activeJob->step();
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
	return activeJob->frameProcessingMode();
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
