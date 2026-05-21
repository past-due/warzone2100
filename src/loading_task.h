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
 * `ResourceLoadingController` drives tasks from `step()` (one resume per frame).
 * Loading coroutines use `co_await controller.yield_frame()` to split work across
 * frames and `co_await child_task` to compose nested loads.
 */

#pragma once

#include "resource_loading_controller.h"

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>

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

	/// Detach handle for `ResourceLoadingController::start` (controller owns destruction).
	std::coroutine_handle<promise_type> release() noexcept
	{
		return std::exchange(coro, {});
	}

	struct ChildTaskAwaiter;

	ChildTaskAwaiter operator co_await() &&;

private:
	friend class ResourceLoadingController;
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
	ResourceLoadingController *controller = nullptr;
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

struct LoadingTaskPromise::FinalAwaiter
{
	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<LoadingTaskPromise> h) const noexcept;

	void await_resume() const noexcept {}
};

inline LoadingTaskPromise::FinalAwaiter LoadingTaskPromise::final_suspend() noexcept
{
	return {};
}

struct LoadingTask::ChildTaskAwaiter
{
	LoadingTask *child = nullptr;
	mutable std::coroutine_handle<LoadingTaskPromise> child_handle{};

	bool await_ready() const noexcept
	{
		return child == nullptr || child->done();
	}

	LoadOutcome await_resume() const noexcept;

	void await_suspend(std::coroutine_handle<> h);
};

inline LoadingTask::ChildTaskAwaiter LoadingTask::operator co_await() &&
{
	return ChildTaskAwaiter{this};
}

/// Cooperative loading job: finalize callbacks + deferred task start on a controller.
class ResourceLoadingJob
{
public:
	using FinalizeCallback = std::function<void()>;

	using TaskFactory = std::function<LoadingTask(ResourceLoadingController &)>;

	ResourceLoadingJob(TaskFactory taskFactory,
	                    FinalizeCallback onSuccess,
	                    FinalizeCallback onFailure,
	                    ResourceLoadingController::FrameProcessingMode initialFrameMode =
	                        ResourceLoadingController::FrameProcessingMode::ConsumeFrame);

	void bindAndStart(ResourceLoadingController &controller);
	LoadStepStatus step(ResourceLoadingController &controller);
	void finalizeSuccess();
	void finalizeFailure();
	ResourceLoadingController::FrameProcessingMode frameProcessingMode() const noexcept
	{
		return initialFrameMode;
	}

private:
	LoadingTask pending_task;
	TaskFactory task_factory;
	FinalizeCallback onSuccess;
	FinalizeCallback onFailure;
	ResourceLoadingController::FrameProcessingMode initialFrameMode =
	    ResourceLoadingController::FrameProcessingMode::ConsumeFrame;
};

std::unique_ptr<ResourceLoadingJob> makeResourceLoadingJob(
    ResourceLoadingJob::TaskFactory taskFactory,
    ResourceLoadingJob::FinalizeCallback onSuccess = {},
    ResourceLoadingJob::FinalizeCallback onFailure = {},
    ResourceLoadingController::FrameProcessingMode initialFrameMode =
        ResourceLoadingController::FrameProcessingMode::ConsumeFrame);

/// Drive any loading job to completion on the current thread (blocking callers).
bool runLoadingJobToCompletion(std::unique_ptr<ResourceLoadingJob> job);
