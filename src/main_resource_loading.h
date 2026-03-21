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
/** \file main_resource_loading.h
 * \brief Phases hooks for starting a new game and loading a save: split so
 * StartGameResourceJob / LoadSaveGameResourceJob can run before / blocking load / after (and failure cleanup)
 * across cooperative loading steps; implementations are in main.cpp in this namespace.
 * The same functions are also used by the synchronous `startGameLoop()` path in `main.cpp`.
 *
 * NOTE: This is a temporary solution to allow the cooperative loading controller to
 * handle the loading of a new game and a save game. It is not a long-term solution
 * and should be replaced with something more architecturally sound later.
 */

#pragma once

#include <memory>

class IResourceLoadingJob;

namespace main_resource_loading
{

std::unique_ptr<IResourceLoadingJob> makeStartGameResourceJob();
std::unique_ptr<IResourceLoadingJob> makeLoadSaveGameResourceJob();

} // namespace main_resource_loading
