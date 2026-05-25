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
 * \brief Cooperative loading scheduler: drive coroutine-based load work from the main loop
 * instead of freezing the game in one long call.
 *
 * Coroutine tasks (see `loading_task.h`) use `co_await controller.yieldFrame()` to
 * split work across frames.
 */

#pragma once

#include <coroutine>
#include <memory>
#include <queue>
#include <stack>
#include <utility>
#include <vector>

class LoadingTask;
struct LoadingTaskPromise;

enum class LoadOutcome
{
	Success,
	Failure,
};

/// Returned by `stepOneQuantum()`.
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

/// Global cooperative loading scheduler: at most one active submission, FIFO queue for pending work.
class ResourceLoadingController
{
public:

	// How `mainLoop` should finish the current iteration after `step()` for the active load.
	enum class FrameProcessingMode
	{
		// End frame after loading step + optional loading UI; skip title/game loop this tick.
		ConsumeFrame,
		// Same frame continues into normal mode (e.g. map preview job).
		ContinueMainLoop,
	};

	/// Per-frame policy for mainLoop; stored on each execution stack entry.
	struct FramePolicy
	{
		FrameProcessingMode frameMode = FrameProcessingMode::ConsumeFrame;
		bool showLoadingScreen = true;
	};

	struct FrameYield;

	static ResourceLoadingController& instance();

	ResourceLoadingController(const ResourceLoadingController&) = delete;
	ResourceLoadingController &operator=(const ResourceLoadingController&) = delete;

	ResourceLoadingController(ResourceLoadingController&&) = delete;
	ResourceLoadingController &operator=(ResourceLoadingController&&) = delete;

	// Submit loading work. If there is an active submission, queue the follow-up;
	// otherwise begin immediately.
	void request(LoadingTask task, FramePolicy policy);

	// Returns true if there is an active submission.
	bool active() const;

	// Advance the active load by one frame quantum.
	void step();

	// Valid only while `active()` and execution is running; reads the execution stack top.
	FrameProcessingMode currentFrameProcessingMode() const;

	// When true, mainLoop presents the loading screen; callback must not flip frames.
	bool loadingScreenHandledByController() const;

	FrameYield yieldFrame() noexcept;

private:

	friend class LoadingTask;
	friend struct LoadingTaskPromise;
	friend struct FrameYield;

	struct ExecutionFrame
	{
		std::coroutine_handle<> handle{};
		ExecutionFrameState state = ExecutionFrameState::Paused;
		FramePolicy policy{};
	};

	struct ResourceLoadingSubmission;

	explicit ResourceLoadingController() = default;
	~ResourceLoadingController();

	void begin(std::unique_ptr<ResourceLoadingSubmission> submission);
	void startNextPendingSubmission();

	void start(LoadingTask task, FramePolicy policy);
	LoadStepStatus stepOneQuantum();
	void completeActiveSubmission(LoadStepStatus result);
	void resetTaskState() noexcept;

	ExecutionFrame &topFrame();
	ExecutionFrame const &topFrame() const;
	void pushFrame(std::coroutine_handle<> handle, FramePolicy policy);
	void popAndDestroyTop() noexcept;
	void onFrameFinished(LoadOutcome outcome) noexcept;
	bool hasActiveExecution() const noexcept { return !executionStack.empty(); }
	bool isExecutingLoadingCoroutine() const noexcept
	{
		return hasActiveExecution()
		    && topFrame().state == ExecutionFrameState::Running;
	}

	std::unique_ptr<ResourceLoadingSubmission> activeSubmission;
	std::queue<std::unique_ptr<ResourceLoadingSubmission>> pendingSubmissions;

	std::stack<ExecutionFrame, std::vector<ExecutionFrame>> executionStack;
	LoadOutcome terminalOutcome = LoadOutcome::Success;
	bool sessionFinished = false;
};

/// `co_await controller.yieldFrame()` — suspend until the next `stepOneQuantum()`.
struct ResourceLoadingController::FrameYield
{
	ResourceLoadingController *controller = nullptr;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept;

	void await_resume() const noexcept {}
};
