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

#include "lib/framework/loading_task.h"
#include "lib/framework/wzapp.h"
#include "wrappers.h"

void submitResourceLoadingTask(ResourceLoadingTaskFactory taskFactory,
                              bool showLoadingScreen,
                              bool drawBackdrop,
                              ResourceLoadingController::FrameProcessingMode frameMode)
{
	ASSERT(taskFactory, "submitResourceLoadingJob given null task factory");
	ResourceLoadingController &controller = ResourceLoadingController::instance();
	if (!controller.active() && showLoadingScreen && !isLoadingScreenActive())
	{
		initLoadingScreen(drawBackdrop);
	}
	ResourceLoadingController::FramePolicy policy;
	policy.showLoadingScreen = showLoadingScreen;
	policy.frameMode = frameMode;
	controller.request(taskFactory(controller), policy);
}

void presentResourceLoadingScreenIfNeeded()
{
	if (ResourceLoadingController::instance().loadingScreenHandledByController())
	{
		presentLoadingScreenForCurrentFrame();
	}
}
