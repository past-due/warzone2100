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
/** \file loading_task.h
 * \brief C++20 coroutine primitives for cooperative resource loading.
 *
 * `LoadingScheduler` drives tasks from `ResourceLoadingController::step()` (one
 * resume per frame). Loading coroutines use `co_await scheduler.yield_frame()` to
 * split work across frames and `co_await child_task` to compose nested loads.
 */

#pragma once

#include "resource_loading_controller.h"

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>

enum class LoadOutcome
{
	Success,
	Failure,
};

/// Mirrors `IResourceLoadingJob::StepResult` for controller integration.
enum class LoadStepStatus
{
	InProgress,
	Completed,
	Failed,
};

class LoadingScheduler;

struct LoadingTaskPromise;

/// Coroutine return type for loading work (`co_return LoadOutcome::...`).
class LoadingTask
{
public:
	using promise_type = LoadingTaskPromise;

	LoadingTask() = default;

	LoadingTask(LoadingTask const &) = delete;
	LoadingTask &operator=(LoadingTask const &) = delete;

	LoadingTask(LoadingTask &&other) noexcept
		: coro(std::exchange(other.coro, {}))
	{
	}

	LoadingTask &operator=(LoadingTask &&other) noexcept
	{
		if (this != &other)
		{
			destroy();
			coro = std::exchange(other.coro, {});
		}
		return *this;
	}

	~LoadingTask() { destroy(); }

	bool empty() const noexcept { return coro == nullptr; }

	bool done() const noexcept { return coro == nullptr || coro.done(); }

	LoadOutcome result() const noexcept;

	/// Detach handle for `LoadingScheduler::start` (scheduler owns destruction).
	std::coroutine_handle<promise_type> release() noexcept
	{
		return std::exchange(coro, {});
	}

	struct NestedAwaiter;

	NestedAwaiter operator co_await() &&;

private:
	friend class LoadingScheduler;
	friend struct LoadingTaskPromise;

	explicit LoadingTask(std::coroutine_handle<promise_type> h) noexcept
		: coro(h)
	{
	}

	void destroy() noexcept
	{
		if (coro)
		{
			coro.destroy();
			coro = {};
		}
	}

	std::coroutine_handle<promise_type> coro{};
};

struct LoadingTaskPromise
{
	LoadingScheduler *scheduler = nullptr;
	LoadOutcome result = LoadOutcome::Success;
	std::exception_ptr exception;

	LoadingTask get_return_object() noexcept
	{
		return LoadingTask{std::coroutine_handle<LoadingTaskPromise>::from_promise(*this)};
	}

	std::suspend_always initial_suspend() noexcept { return {}; }

	struct FinalAwaiter;

	FinalAwaiter final_suspend() noexcept;

	void return_value(LoadOutcome value) noexcept { result = value; }

	void unhandled_exception() noexcept
	{
		exception = std::current_exception();
		result = LoadOutcome::Failure;
	}
};

/// Cooperative scheduler: one `step_one_quantum()` resumes the active coroutine once.
class LoadingScheduler
{
public:
	struct FrameYield;
	struct Bind;
	struct SetFrameMode;

	FrameYield yield_frame() noexcept;
	Bind bind() noexcept;
	SetFrameMode set_frame_mode(ResourceLoadingController::FrameProcessingMode mode) noexcept;

	/// Install the root task; must be called before the first `step_one_quantum()`.
	/// Binds the root promise to this scheduler (no `co_await bind()` needed for root tasks).
	void start(LoadingTask task);

	void set_frame_processing_mode(ResourceLoadingController::FrameProcessingMode mode) noexcept
	{
		frame_mode = mode;
	}

	/// Resume the active coroutine once. Returns terminal status when the root task finishes.
	LoadStepStatus step_one_quantum();

	bool active() const noexcept { return root_coro != nullptr && !task_finished; }
	bool is_finished() const noexcept { return task_finished; }
	LoadOutcome outcome() const noexcept { return root_outcome; }

	ResourceLoadingController::FrameProcessingMode frame_processing_mode() const noexcept
	{
		return frame_mode;
	}

private:
	friend class LoadingTask;
	friend struct LoadingTaskPromise;
	friend struct FrameYield;
	friend struct Bind;
	friend struct SetFrameMode;

	void on_root_task_finished(LoadOutcome result) noexcept;
	void on_nested_child_finished() noexcept;
	void push_nested(std::coroutine_handle<LoadingTaskPromise> child,
	                 std::coroutine_handle<> parent) noexcept;

	std::coroutine_handle<> current{};
	std::coroutine_handle<> parent_coro{};
	std::coroutine_handle<LoadingTaskPromise> root_coro{};
	LoadOutcome root_outcome = LoadOutcome::Success;
	bool task_finished = false;
	ResourceLoadingController::FrameProcessingMode frame_mode =
	    ResourceLoadingController::FrameProcessingMode::ConsumeFrame;
};

struct LoadingTaskPromise::FinalAwaiter
{
	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<LoadingTaskPromise> h) const noexcept
	{
		auto &promise = h.promise();
		if (promise.scheduler == nullptr)
		{
			return;
		}

		if (h == promise.scheduler->root_coro)
		{
			promise.scheduler->on_root_task_finished(promise.result);
		}
		else
		{
			promise.scheduler->on_nested_child_finished();
		}
	}

	void await_resume() const noexcept {}
};

inline LoadingTaskPromise::FinalAwaiter LoadingTaskPromise::final_suspend() noexcept
{
	return {};
}

/// `co_await scheduler.yield_frame()` — suspend until the next `step_one_quantum()`.
struct LoadingScheduler::FrameYield
{
	LoadingScheduler *scheduler = nullptr;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept
	{
		scheduler->current = h;
	}

	void await_resume() const noexcept {}
};

/// `co_await scheduler.bind()` — attach this coroutine to the scheduler (first line of a task).
struct LoadingScheduler::Bind
{
	LoadingScheduler *scheduler = nullptr;

	bool await_ready() const noexcept { return false; }

	template<typename Promise>
	void await_suspend(std::coroutine_handle<Promise> h) const noexcept
	{
		h.promise().scheduler = scheduler;
		scheduler->current = h;
	}

	void await_resume() const noexcept {}
};

/// `co_await scheduler.set_frame_mode(...)` — policy for the current main-loop frame.
struct LoadingScheduler::SetFrameMode
{
	LoadingScheduler *scheduler = nullptr;
	ResourceLoadingController::FrameProcessingMode mode =
	    ResourceLoadingController::FrameProcessingMode::ConsumeFrame;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> /*h*/) const noexcept
	{
		scheduler->frame_mode = mode;
	}

	void await_resume() const noexcept {}
};

struct LoadingTask::NestedAwaiter
{
	LoadingTask *child = nullptr;
	std::coroutine_handle<> parent{};

	bool await_ready() const noexcept
	{
		return child == nullptr || child->done();
	}

	LoadOutcome await_resume() const noexcept
	{
		return child ? child->result() : LoadOutcome::Failure;
	}

	void await_suspend(std::coroutine_handle<> h);
};

inline LoadingTask::NestedAwaiter LoadingTask::operator co_await() &&
{
	return NestedAwaiter{this};
}

/// `IResourceLoadingJob` adapter: drives a `LoadingTask` via `LoadingScheduler`.
class CoroutineLoadingJob final : public IResourceLoadingJob
{
public:
	using FinalizeCallback = std::function<void()>;

	using TaskFactory = std::function<LoadingTask(LoadingScheduler &)>;

	CoroutineLoadingJob(LoadingTask task, FinalizeCallback onSuccess, FinalizeCallback onFailure);
	CoroutineLoadingJob(TaskFactory taskFactory,
	                    FinalizeCallback onSuccess,
	                    FinalizeCallback onFailure,
	                    ResourceLoadingController::FrameProcessingMode initialFrameMode =
	                        ResourceLoadingController::FrameProcessingMode::ConsumeFrame);

	StepResult step() override;
	void finalizeSuccess() override;
	void finalizeFailure() override;
	ResourceLoadingController::FrameProcessingMode frameProcessingMode() const override;

private:
	LoadingScheduler scheduler;
	FinalizeCallback onSuccess;
	FinalizeCallback onFailure;
};

std::unique_ptr<IResourceLoadingJob> makeCoroutineLoadingJob(
    LoadingTask task,
    CoroutineLoadingJob::FinalizeCallback onSuccess = {},
    CoroutineLoadingJob::FinalizeCallback onFailure = {});

std::unique_ptr<IResourceLoadingJob> makeCoroutineLoadingJob(
    CoroutineLoadingJob::TaskFactory taskFactory,
    CoroutineLoadingJob::FinalizeCallback onSuccess = {},
    CoroutineLoadingJob::FinalizeCallback onFailure = {},
    ResourceLoadingController::FrameProcessingMode initialFrameMode =
        ResourceLoadingController::FrameProcessingMode::ConsumeFrame);

/// Convenience: `makeCoroutineLoadingJob` + `ResourceLoadingController::request`.
void requestCoroutineLoad(ResourceLoadingController &controller,
                          ResourceLoadingRequest request,
                          LoadingTask task,
                          CoroutineLoadingJob::FinalizeCallback finalizeSuccess = {},
                          CoroutineLoadingJob::FinalizeCallback finalizeFailure = {});

/// Drive any loading job to completion on the current thread (for blocking callers).
bool runLoadingJobToCompletion(IResourceLoadingJob &job);
