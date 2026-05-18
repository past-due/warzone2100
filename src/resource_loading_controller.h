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
/** \file resource_loading_controller.h
 * \brief Cooperative loading: schedule heavy load work from the main loop
 * instead of freezing the game in one long call (or be dependent on
 * `RESLOAD_CALLBACK` deep within the call chains to update the loading screen
 * frames).
 *
 * Describes what to load (`frontendInit`, `startGame`, `loadSaveGame`, `mapPreview`)
 * and exposes the controller API used to submit work and drive progress each frame,
 * including when to show the normal loading screen.
 */

#pragma once

#include "lib/framework/crc.h"

#include <memory>
#include <optional>
#include <string>

class IResourceLoadingJob;

/// <summary>
/// Describes one unit of loading work submitted to the resource loading controller.
///
/// NOTE: uses only a subset of the fields specific to each kind of loading request.
/// </summary>
struct ResourceLoadingRequest
{
	enum class Kind
	{
		FrontendInit,
		StartGame,
		LoadSaveGame,
		MapPreview,
	};

	Kind kind;
	bool drawBackdrop = true;
	bool showLoadingScreen = true;
	std::string resourceFile;
	bool onInitialStartup = false;
	bool hideInterface = false;
	std::string previewMapName;
	Sha256 previewMapHash;

	static ResourceLoadingRequest frontendInit(bool onInitialStartup = false)
	{
		ResourceLoadingRequest request;
		request.kind = Kind::FrontendInit;
		request.drawBackdrop = !onInitialStartup;
		request.showLoadingScreen = !onInitialStartup;
		request.resourceFile = "wrf/frontend.wrf";
		request.onInitialStartup = onInitialStartup;
		return request;
	}

	static ResourceLoadingRequest startGame()
	{
		ResourceLoadingRequest request;
		request.kind = Kind::StartGame;
		return request;
	}

	static ResourceLoadingRequest loadSaveGame()
	{
		ResourceLoadingRequest request;
		request.kind = Kind::LoadSaveGame;
		return request;
	}

	static ResourceLoadingRequest mapPreview(bool hideInterface)
	{
		ResourceLoadingRequest request;
		request.kind = Kind::MapPreview;
		request.drawBackdrop = false;
		request.showLoadingScreen = false;
		request.hideInterface = hideInterface;
		return request;
	}

	static ResourceLoadingRequest mapPreview(bool hideInterface, std::string mapName, Sha256 mapHash)
	{
		ResourceLoadingRequest request = mapPreview(hideInterface);
		request.previewMapName = std::move(mapName);
		request.previewMapHash = mapHash;
		return request;
	}
};

/// <summary>
/// Global cooperative loading scheduler used from `mainLoop`: at most one active job,
/// optional single queued follow-up `request`. `step` runs once per frame while active.
/// Jobs may consume the whole frame or allow the normal title/game loop to run afterward
/// (`FrameProcessingMode`).
///
/// Loading screen UI is driven here when `showLoadingScreen` is set.
/// </summary>
class ResourceLoadingController
{
public:

	// How `mainLoop` should finish the current iteration after `step()` for the active job.
	enum class FrameProcessingMode
	{
		// End frame after loading step + optional loading UI; skip title/game loop this tick.
		ConsumeFrame,
		// Same frame continues into normal mode (e.g. map preview job).
		ContinueMainLoop,
	};

	static ResourceLoadingController &instance();

	explicit ResourceLoadingController() = default;
	~ResourceLoadingController() = default;

	ResourceLoadingController(const ResourceLoadingController&) = delete;
	ResourceLoadingController &operator=(const ResourceLoadingController&) = delete;

	ResourceLoadingController(ResourceLoadingController&&) = delete;
	ResourceLoadingController &operator=(ResourceLoadingController&&) = delete;

	// Submit a new loading request. If there is an active job, queue the request;
	// otherwise begin a new job with the request.
	void request(ResourceLoadingRequest request);

	/// Submit a pre-built job (e.g. from `makeCoroutineLoadingJob` in `loading_task.h`).
	/// Same queueing rules as `request(ResourceLoadingRequest)`.
	void request(ResourceLoadingRequest request, std::unique_ptr<IResourceLoadingJob> job);

	// Returns true if there is an active job.
	bool active() const;

	// Advance the active job's state machine.
	void step();

	// Present the loading screen if needed.
	void presentLoadingScreenIfNeeded();

	// Valid only while `active()`; reflects the active job's policy for this frame.
	FrameProcessingMode currentFrameProcessingMode() const;

	// When true, mainLoop presents the loading screen; callback must not flip frames.
	bool loadingScreenHandledByController() const;

private:

	void begin(ResourceLoadingRequest request, std::unique_ptr<IResourceLoadingJob> job = nullptr);
	static std::unique_ptr<IResourceLoadingJob> makeJob(const ResourceLoadingRequest &request);

	std::optional<ResourceLoadingRequest> activeRequest;
	// NOTE: It makes sense to support queueing only a single request at a time,
	// since resource loading jobs represent coarse-grained loading steps (additional
	// work that'll be made on behalf of the currently active job is represented
	// by a new sub-job belonging to the active job).
	//
	// So it's absolutely sufficient to only store the next loading step in the
	// controller's state machine.
	std::optional<ResourceLoadingRequest> queuedRequest;
	std::unique_ptr<IResourceLoadingJob> queuedJob;
	std::unique_ptr<IResourceLoadingJob> activeJob;
};

/// <summary>
/// Cooperative loading job: a small per-frame state machine.
/// `ResourceLoadingController::step()` calls `step()` at most once per main-loop
/// iteration while the job is active.
///
/// Each `step()` performs part of the work, advances an internal phase (or leaves
/// it unchanged), and returns:
/// * `InProgress`  — keep the job; controller will call `step()` again on a later frame.
/// * `Completed`   — controller calls `finalizeSuccess()` once, then destroys the job.
/// * `Failed`      — controller calls `finalizeFailure()` once, then destroys the job.
///
/// Do not assume multiple `step()` calls in the same frame. Heavy work should be
/// split across phases (or use a nested cooperative API like `resLoadPlanStep`)
/// so the UI can update between frames.
///
/// `frameProcessingMode()` tells `mainLoop` whether this frame ends after loading UI
/// only (`ConsumeFrame`) or may continue into the normal title/game loop (`ContinueMainLoop`).
/// </summary>
class IResourceLoadingJob
{
public:
	enum class StepResult
	{
		InProgress,
		Completed,
		Failed,
	};

	virtual ~IResourceLoadingJob() = default;
	virtual StepResult step() = 0;
	virtual void finalizeSuccess() = 0;
	virtual void finalizeFailure() = 0;
	virtual ResourceLoadingController::FrameProcessingMode frameProcessingMode() const
	{
		return ResourceLoadingController::FrameProcessingMode::ConsumeFrame;
	}
};
