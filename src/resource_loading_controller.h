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
 * Exposes the controller API used to submit work and drive progress each frame,
 * including when to show the normal loading screen.
 *
 * Coroutine tasks (see `loading_task.h`) use `co_await controller.yield_frame()` to
 * split work across frames.
 *
 * Game-specific request types live in `resource_loading_request.h`.
 */

#pragma once

#include "resource_loading_request.h"

#include <coroutine>
#include <memory>
#include <optional>
#include <stack>
#include <vector>

class ResourceLoadingJob;
class LoadingTask;
struct LoadingTaskPromise;

enum class LoadOutcome
{
	Success,
	Failure,
};

/// Returned by `stepOneQuantum()` / `ResourceLoadingJob::step()`.
enum class LoadStepStatus
{
	InProgress,
	Completed,
	Failed,
};

enum class ExecutionFrameState
{
	Paused,
	Running,
};

struct ExecutionFrame
{
	std::coroutine_handle<> handle{};
	ExecutionFrameState state = ExecutionFrameState::Paused;
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

	static ResourceLoadingController& instance();

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

	FrameYield yieldFrame() noexcept;

	void setFrameProcessingMode(FrameProcessingMode mode) noexcept { frameMode = mode; }

	FrameProcessingMode frameProcessingMode() const noexcept { return frameMode; }

private:

	friend class LoadingTask;
	friend struct LoadingTaskPromise;
	friend class ResourceLoadingJob;
	friend struct FrameYield;

	explicit ResourceLoadingController() = default;
	~ResourceLoadingController() = default;

	void begin(ResourceLoadingRequest request);
	static std::unique_ptr<ResourceLoadingJob> makeJob(const ResourceLoadingRequest &request);

	void start(LoadingTask task);
	LoadStepStatus stepOneQuantum();
	void resetTaskState() noexcept;

	ExecutionFrame &topFrame();
	ExecutionFrame const &topFrame() const;
	void pushFrame(std::coroutine_handle<> handle);
	void popAndDestroyTop() noexcept;
	void onFrameFinished(LoadOutcome outcome) noexcept;
	bool hasActiveExecution() const noexcept { return !executionStack.empty(); }

	std::optional<ResourceLoadingRequest> activeRequest;
	std::optional<ResourceLoadingRequest> queuedRequest;
	std::unique_ptr<ResourceLoadingJob> activeJob;

	std::stack<ExecutionFrame, std::vector<ExecutionFrame>> executionStack;
	LoadOutcome terminalOutcome = LoadOutcome::Success;
	bool sessionFinished = false;
	FrameProcessingMode frameMode = FrameProcessingMode::ConsumeFrame;
};

/// `co_await controller.yield_frame()` — suspend until the next `step_one_quantum()`.
struct ResourceLoadingController::FrameYield
{
	ResourceLoadingController *controller = nullptr;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept;

	void await_resume() const noexcept {}
};
