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
/** \file resource_loading_request.h
 * \brief Game-specific loading request descriptors for the cooperative loader.
 *
 * Describes what to load (`frontendInit`, `startGame`, `loadSaveGame`, `mapPreview`).
 * Each kind uses only a subset of the fields below.
 */

#pragma once

#include "lib/framework/crc.h"

#include <string>

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
