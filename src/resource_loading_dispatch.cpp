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
/** \file resource_loading_dispatch.cpp
 * \brief Game-level loading submission implementation.
 */

#include "resource_loading_dispatch.h"

#include "init.h"
#include "lib/framework/loading_task.h"
#include "lib/framework/resource_loading_controller.h"
#include "main_resource_loading.h"
#include "multiint.h"
#include "wrappers.h"

#include <optional>
#include <utility>

namespace
{

std::optional<ResourceLoadingRequest> queuedRequest;

std::unique_ptr<ResourceLoadingJob> makeJob(const ResourceLoadingRequest &request)
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

void beginResourceLoading(ResourceLoadingRequest request)
{
	ResourceLoadingController &controller = ResourceLoadingController::instance();
	ASSERT(!controller.active(), "beginResourceLoading called while another loading job is active");

	const bool hadLoadingScreen = isLoadingScreenActive();
	if (request.showLoadingScreen && !hadLoadingScreen)
	{
		initLoadingScreen(request.drawBackdrop);
	}

	std::unique_ptr<ResourceLoadingJob> job = makeJob(request);
	ASSERT(job, "Failed to create loading job");
	controller.request(std::move(job), request.showLoadingScreen);
}

} // namespace

void requestResourceLoading(ResourceLoadingRequest request)
{
	if (ResourceLoadingController::instance().active())
	{
		queuedRequest = std::move(request);
		return;
	}

	beginResourceLoading(std::move(request));
}

void processResourceLoadingQueue()
{
	if (ResourceLoadingController::instance().active() || !queuedRequest.has_value())
	{
		return;
	}

	ResourceLoadingRequest nextRequest = std::move(queuedRequest.value());
	queuedRequest.reset();
	beginResourceLoading(std::move(nextRequest));
}

void presentResourceLoadingScreenIfNeeded()
{
	if (ResourceLoadingController::instance().loadingScreenHandledByController())
	{
		presentLoadingScreenForCurrentFrame();
	}
}
