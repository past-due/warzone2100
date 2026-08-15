// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#pragma once

#include <functional>

#include <nlohmann/json_fwd.hpp>

// Assembles the structured BuildInfo blob: self-reported build metadata plus the self-measured executable hash.
//
// Schema (version 1):
// {
//   "schema": 1,
//   "version": "<version string>",
//   "platform": "<platform string>",
//   "distributor": "<package distributor>",
//   "git": { "tag": "...", "commit": "<full hash>", "dirty": <bool> },
//   "exe": {
//     "path_confidence": "kernel" | "heuristic",
//     "size": <bytes>,
//     "hashes": { "raw_sha256": "<hex>" }
//   }
// }
//
// If hashing failed, "exe" contains "error" instead of "size"/"hashes".

// Obtain the BuildInfo blob.
// `resultFunc` is always invoked asynchronously on the main thread.
// NOTE: Main thread only.
void getBuildInfo(std::function<void(const nlohmann::ordered_json&)> resultFunc);
