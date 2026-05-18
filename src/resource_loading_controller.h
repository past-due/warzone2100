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
 *
 * Coroutine tasks (see `loading_task.h`) use `co_await controller.yield_frame()` to
 * split work across frames.
 */

#pragma once

#include "lib/framework/crc.h"

#include <coroutine>
#include <memory>
#include <optional>
#include <string>

class ResourceLoadingJob;
class LoadingTask;
struct LoadingTaskPromise;

enum class LoadOutcome
{
	Success,
	Failure,
};

/// Returned by `step_one_quantum()` / `ResourceLoadingJob::step()`.
enum class LoadStepStatus
{
	InProgress,
	Completed,
	Failed,
};

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

	struct FrameYield;

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

	/// Run a job to completion on the current thread (blocking). Uses this controller's
	/// coroutine state; do not call on `instance()` while `active()` on the singleton.
	bool runJobToCompletion(std::unique_ptr<ResourceLoadingJob> job);

	FrameYield yieldFrame() noexcept;

	void setFrameProcessingMode(FrameProcessingMode mode) noexcept { frameMode = mode; }

	FrameProcessingMode frameProcessingMode() const noexcept { return frameMode; }

private:
	friend class LoadingTask;
	friend struct LoadingTaskPromise;
	friend class ResourceLoadingJob;
	friend struct FrameYield;

	void begin(ResourceLoadingRequest request, std::unique_ptr<ResourceLoadingJob> job = nullptr);
	static std::unique_ptr<ResourceLoadingJob> makeJob(const ResourceLoadingRequest &request);

	void start(LoadingTask task);
	LoadStepStatus stepOneQuantum();
	void resetTaskState() noexcept;
	void onRootTaskFinished(LoadOutcome result) noexcept;
	void onNestedChildFinished() noexcept;
	void pushNested(std::coroutine_handle<LoadingTaskPromise> child, std::coroutine_handle<> parent) noexcept;

	std::optional<ResourceLoadingRequest> activeRequest;
	std::optional<ResourceLoadingRequest> queuedRequest;
	std::unique_ptr<ResourceLoadingJob> queuedJob;
	std::unique_ptr<ResourceLoadingJob> activeJob;

	std::coroutine_handle<> current{};
	std::coroutine_handle<> parentCoro{};
	std::coroutine_handle<LoadingTaskPromise> rootCoro{};
	LoadOutcome root_outcome = LoadOutcome::Success;
	bool taskFinished = false;
	FrameProcessingMode frameMode = FrameProcessingMode::ConsumeFrame;
};

/// `co_await controller.yield_frame()` — suspend until the next `step_one_quantum()`.
struct ResourceLoadingController::FrameYield
{
	ResourceLoadingController *controller = nullptr;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept
	{
		controller->current = h;
	}

	void await_resume() const noexcept {}
};
