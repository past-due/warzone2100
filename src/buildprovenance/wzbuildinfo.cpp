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

#include "wzbuildinfo.h"
#include "wzbindetails.h"

#include "lib/framework/frame.h"

#include "../wzpropertyproviders.h"

#include <nlohmann/json.hpp>

#include <utility>

namespace
{

std::string buildProperty(BuildPropertyProvider& provider, const char* name)
{
	std::string value;
	if (!provider.getPropertyValue(name, value))
	{
		value.clear();
	}
	return value;
}

nlohmann::ordered_json assembleBuildInfo(const ExeDetails& exeDetails)
{
	BuildPropertyProvider buildProps;

	nlohmann::ordered_json info = nlohmann::ordered_json::object();
	info["schema"] = 1;
	info["version"] = buildProperty(buildProps, "VERSION_STRING");
	info["platform"] = buildProperty(buildProps, "PLATFORM");
	info["distributor"] = buildProperty(buildProps, "WZ_PACKAGE_DISTRIBUTOR");

	nlohmann::ordered_json git = nlohmann::ordered_json::object();
	git["tag"] = buildProperty(buildProps, "GIT_TAG");
	git["commit"] = buildProperty(buildProps, "GIT_FULL_HASH");
	std::string wcModified = buildProperty(buildProps, "GIT_WC_MODIFIED");
	git["dirty"] = (!wcModified.empty() && wcModified != "0");
	info["git"] = std::move(git);

	nlohmann::ordered_json exe = nlohmann::ordered_json::object();
	if (exeDetails.rawHash.has_value())
	{
		exe["path_confidence"] = (exeDetails.pathConfidence == HashableFile::PathConfidence::KernelAuthoritative) ? "kernel" : "heuristic";
		exe["size"] = exeDetails.fileSize.value_or(0);
		nlohmann::ordered_json hashes = nlohmann::ordered_json::object();
		hashes["raw_sha256"] = exeDetails.rawHash.value().toString();
		exe["hashes"] = std::move(hashes);
	}
	else
	{
		exe["error"] = exeDetails.errorDetails;
	}
	info["exe"] = std::move(exe);

	return info;
}

} // anonymous namespace

void getBuildInfo(std::function<void(const nlohmann::ordered_json&)> resultFunc)
{
	ASSERT_OR_RETURN(, resultFunc != nullptr, "Null resultFunc");
	getSelfExecutableDetails([resultFunc](const ExeDetails& exeDetails) {
		resultFunc(assembleBuildInfo(exeDetails));
	});
}
