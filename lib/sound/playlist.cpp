/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/physfs_ext.h"

#include "playlist.h"
#include "cdaudio.h"

#include <3rdparty/json/json.hpp>
#include <algorithm>
#include <unordered_map>

#include <optional-lite/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

struct WZ_TRACK_SETTINGS
{
	bool default_music_modes[NUM_MUSICGAMEMODES] = {false};
	bool music_modes[NUM_MUSICGAMEMODES] = {false};
};

static std::vector<std::shared_ptr<WZ_ALBUM>> albumList;
static std::vector<std::shared_ptr<const WZ_TRACK>> fullTrackList;
typedef std::unordered_map<std::shared_ptr<const WZ_TRACK>, WZ_TRACK_SETTINGS> TRACK_SETTINGS_MAP;
static TRACK_SETTINGS_MAP trackToSettingsMap;
static optional<size_t> currentSong = nullopt;
static MusicGameMode lastFilteredMode = MusicGameMode::MENUS;
static std::function<bool (const std::shared_ptr<const WZ_TRACK>& track)> currentFilterFunc;

void PlayList_Init()
{
	albumList.clear();
	fullTrackList.clear();
	trackToSettingsMap.clear();
	currentFilterFunc = nullptr;
	currentSong = nullopt;
}

void PlayList_Quit()
{
	PlayList_Init();
}

bool PlayList_TrackEnabledForMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		return false;
	}
	return it->second.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)];
}

void PlayList_SetTrackMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode, bool enabled)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		return;
	}
	it->second.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
}

static void PlayList_SetTrackDefaultMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode, bool enabled)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		auto result = trackToSettingsMap.insert(TRACK_SETTINGS_MAP::value_type(track, WZ_TRACK_SETTINGS()));
		it = result.first;
	}
	it->second.default_music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
	it->second.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
}

const std::vector<std::shared_ptr<const WZ_TRACK>>& PlayList_GetFullTrackList()
{
	return fullTrackList;
}

size_t PlayList_SetTrackFilter(const std::function<bool (const std::shared_ptr<const WZ_TRACK>& track)>& filterFunc)
{
	size_t tracksEnabled = 0;
	for (const auto& album : albumList)
	{
		for (const auto& track : album->tracks)
		{
			if (filterFunc(track))
			{
				++tracksEnabled;
			}
		}
	}
	currentFilterFunc = filterFunc;
	return tracksEnabled;
}

size_t PlayList_FilterByMusicMode(MusicGameMode currentMode)
{
	size_t result = PlayList_SetTrackFilter([currentMode](const std::shared_ptr<const WZ_TRACK>& track) -> bool {
		return PlayList_TrackEnabledForMusicMode(track, currentMode);
	});
	if (lastFilteredMode != currentMode)
	{
		currentSong = nullopt;
	}
	lastFilteredMode = currentMode;
	return result;
}

MusicGameMode PlayList_GetCurrentMusicMode()
{
	return lastFilteredMode;
}

static std::shared_ptr<WZ_ALBUM> PlayList_LoadAlbum(const nlohmann::json& json, const std::string& sourcePath, const std::string& albumDir)
{
	ASSERT(!json.is_null(), "JSON album from %s is null", sourcePath.c_str());
	ASSERT(json.is_object(), "JSON album from %s is not an object", sourcePath.c_str());

	std::shared_ptr<WZ_ALBUM> album = std::make_shared<WZ_ALBUM>();

#define GET_REQUIRED_ALBUM_KEY_STRVAL(KEY) \
	if (!json.contains(#KEY)) \
	{ \
		debug(LOG_ERROR, "%s: Missing required `" #KEY "` key", sourcePath.c_str()); \
		return nullptr; \
	} \
	album->KEY = json[#KEY].get<std::string>();

	GET_REQUIRED_ALBUM_KEY_STRVAL(title);
	GET_REQUIRED_ALBUM_KEY_STRVAL(author);
	GET_REQUIRED_ALBUM_KEY_STRVAL(date);
	GET_REQUIRED_ALBUM_KEY_STRVAL(description);
	GET_REQUIRED_ALBUM_KEY_STRVAL(album_cover_filename);
	if (!album->album_cover_filename.empty())
	{
		album->album_cover_filename = albumDir + "/" + album->album_cover_filename;
	}

	if (!json.contains("tracks"))
	{
		debug(LOG_ERROR, "Missing required `tracks` key");
		return nullptr;
	}
	if (!json["tracks"].is_array())
	{
		debug(LOG_ERROR, "Required key `tracks` should be an array");
		return nullptr;
	}

#define GET_REQUIRED_TRACK_KEY(KEY, TYPE) \
	if (!trackJson.contains(#KEY)) \
	{ \
		debug(LOG_ERROR, "%s: Missing required track key: `" #KEY "`", sourcePath.c_str()); \
		return nullptr; \
	} \
	track->KEY = trackJson[#KEY].get<TYPE>();

	for (auto& trackJson : json["tracks"])
	{
		std::shared_ptr<WZ_TRACK> track = std::make_shared<WZ_TRACK>();
		GET_REQUIRED_TRACK_KEY(filename, std::string);
		if (track->filename.empty())
		{
			// Track must have a filename
			debug(LOG_ERROR, "%s: Empty or invalid filename for track", sourcePath.c_str());
			return nullptr;
		}
		track->filename = albumDir + "/" + track->filename;
		GET_REQUIRED_TRACK_KEY(title, std::string);
		GET_REQUIRED_TRACK_KEY(author, std::string);
		GET_REQUIRED_TRACK_KEY(base_volume, unsigned int);
		GET_REQUIRED_TRACK_KEY(bpm, unsigned int);
		track->album = std::weak_ptr<WZ_ALBUM>(album);

		// music_modes // TODO: Only process the "default" modes if they aren't loaded from saved settings for this track
		if (!trackJson.contains("default_music_modes"))
		{
			debug(LOG_ERROR, "%s: Missing required track key: `default_music_modes`", sourcePath.c_str());
			return nullptr;
		}
		if (!trackJson["default_music_modes"].is_array())
		{
			debug(LOG_ERROR, "%s: Required track key `default_music_modes` should be an array", sourcePath.c_str());
			return nullptr;
		}
		for (const auto& musicModeJSON : trackJson["default_music_modes"])
		{
			if (!musicModeJSON.is_string())
			{
				debug(LOG_ERROR, "%s: `default_music_modes` array item should be a string", sourcePath.c_str());
				continue;
			}
			std::string musicMode = musicModeJSON.get<std::string>();
			if (musicMode == "campaign")
			{
				PlayList_SetTrackDefaultMusicMode(track, MusicGameMode::CAMPAIGN, true);
			}
			else if (musicMode == "challenge")
			{
				PlayList_SetTrackDefaultMusicMode(track, MusicGameMode::CHALLENGE, true);
			}
			else if (musicMode == "skirmish")
			{
				PlayList_SetTrackDefaultMusicMode(track, MusicGameMode::SKIRMISH, true);
			}
			else if (musicMode == "multiplayer")
			{
				PlayList_SetTrackDefaultMusicMode(track, MusicGameMode::MULTIPLAYER, true);
			}
			else
			{
				debug(LOG_WARNING, "%s: `default_music_modes` array item is unknown value: %s", sourcePath.c_str(), musicMode.c_str());
				continue;
			}
		}
		album->tracks.push_back(std::move(track));
	}

	return album;
}

bool PlayList_Read(const char *path)
{
	UDWORD size = 0;
	char *data = nullptr;
	std::string albumsPath = astringf("%s/albums", path);

	char **pAlbumFolderList = PHYSFS_enumerateFiles(albumsPath.c_str());
	for (char **i = pAlbumFolderList; *i != nullptr; i++)
	{
		std::string albumDir = albumsPath + "/" + *i;
		std::string str = albumDir + "/album.json";
		if (!PHYSFS_exists(str.c_str()))
		{
			continue;
		}
		if (!loadFile(str.c_str(), &data, &size))
		{
			debug(LOG_ERROR, "album JSON file \"%s\" could not be opened!", str.c_str());
			continue;
		}
		nlohmann::json tmpJson;
		try {
			tmpJson = nlohmann::json::parse(data, data + size);
		}
		catch (const std::exception &e) {
			debug(LOG_ERROR, "album JSON file %s is invalid: %s", str.c_str(), e.what());
			continue;
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception parsing album JSON from %s", str.c_str());
		}
		ASSERT(!tmpJson.is_null(), "JSON album from %s is null", str.c_str());
		ASSERT(tmpJson.is_object(), "JSON album from %s is not an object. Read: \n%s", str.c_str(), data);
		free(data);

		// load album data
		auto album = PlayList_LoadAlbum(tmpJson, str, albumDir);
		if (!album)
		{
			debug(LOG_ERROR, "Failed to load album JSON: %s", str.c_str());
			continue;
		}
		albumList.push_back(std::move(album));
	}
	PHYSFS_freeList(pAlbumFolderList);

	// Ensure that "Warzone 2100 OST" is always the first album in the list
	std::stable_sort(albumList.begin(), albumList.end(), [](const std::shared_ptr<WZ_ALBUM>& a, const std::shared_ptr<WZ_ALBUM>& b) -> bool {
		if (a == b) { return false; }
		if (a->title == "Warzone 2100 OST") { return true; }
		return false;
	});

	// generate full track list
	fullTrackList.clear();
	for (const auto& album : albumList)
	{
		for (const auto& track : album->tracks)
		{
			fullTrackList.push_back(track);
		}
	}
	currentSong = nullopt;

	return !albumList.empty();
}

static optional<size_t> PlayList_FindNextMatchingTrack(size_t startingSongIdx)
{
	size_t idx = (startingSongIdx < (fullTrackList.size() - 1)) ? (startingSongIdx + 1) : 0;
	while (idx != startingSongIdx)
	{
		if (currentFilterFunc(fullTrackList[idx]))
		{
			return idx;
		}
		idx = (idx < (fullTrackList.size() - 1)) ? (idx + 1) : 0;
	}
	return nullopt;
}

std::shared_ptr<const WZ_TRACK> PlayList_CurrentSong()
{
	if (fullTrackList.empty())
	{
		return nullptr;
	}
	if (!currentSong.has_value() || (currentSong.value() >= fullTrackList.size()))
	{
		currentSong = PlayList_FindNextMatchingTrack(fullTrackList.size() - 1);
		if (!currentSong.has_value())
		{
			return nullptr;
		}
	}
	return fullTrackList[currentSong.value()];
}

std::shared_ptr<const WZ_TRACK> PlayList_NextSong()
{
	size_t currentSongIdx = currentSong.has_value() ? currentSong.value() : (fullTrackList.size() - 1);
	currentSong = PlayList_FindNextMatchingTrack(currentSongIdx);
	return PlayList_CurrentSong();
}

bool PlayList_SetCurrentSong(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (!track) { return false; }
	optional<size_t> trackIdx = nullopt;
	for (size_t i = 0; i < fullTrackList.size(); i++)
	{
		if (track == fullTrackList[i])
		{
			trackIdx = i;
			break;
		}
	}
	currentSong = trackIdx;
	return trackIdx.has_value();
}
