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

#include <memory>
#include <3rdparty/json/json.hpp>

struct WZ_ALBUM; // forward-declare

struct WZ_TRACK
{
	std::string filename;
	std::string title;
	std::string author;
	bool music_modes[NUM_MUSICGAMEMODES] = {false};
	unsigned int base_volume = 100;
	unsigned int bpm = 80;
	std::weak_ptr<WZ_ALBUM> album;

	bool enabledForMusicMode(MusicGameMode mode) const
	{
		return music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)];
	}
	void setMusicMode(MusicGameMode mode, bool enabled)
	{
		music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
	}
};

struct WZ_ALBUM
{
	std::string title;
	std::string author;
	std::string date;
	std::string description;
	std::string album_cover_filename;
	std::vector<std::shared_ptr<WZ_TRACK>> tracks;
};


static std::vector<std::shared_ptr<WZ_ALBUM>> albumList;
static std::vector<std::shared_ptr<WZ_TRACK>> playList;
static size_t currentSong = 0;
static MusicGameMode lastFilteredMode = MusicGameMode::MENUS;

void PlayList_Init()
{
	albumList.clear();
	playList.clear();
	currentSong = 0;
}

void PlayList_Quit()
{
	PlayList_Init();
}

size_t PlayList_SetTrackFilter(const std::function<bool (const WZ_TRACK& track)>& filterFunc)
{
	playList.clear();
	for (const auto& album : albumList)
	{
		for (const auto& track : album->tracks)
		{
			if (filterFunc(*track.get()))
			{
				playList.push_back(track);
			}
		}
	}
	return playList.size();
}

size_t PlayList_FilterByMusicMode(MusicGameMode currentMode)
{
	size_t result = PlayList_SetTrackFilter([currentMode](const WZ_TRACK& track) -> bool {
		return track.enabledForMusicMode(currentMode);
	});
	if (lastFilteredMode != currentMode)
	{
		currentSong = 0;
	}
	lastFilteredMode = currentMode;
	return result;
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
				track->setMusicMode(MusicGameMode::CAMPAIGN, true);
			}
			else if (musicMode == "challenge")
			{
				track->setMusicMode(MusicGameMode::CHALLENGE, true);
			}
			else if (musicMode == "skirmish")
			{
				track->setMusicMode(MusicGameMode::SKIRMISH, true);
			}
			else if (musicMode == "multiplayer")
			{
				track->setMusicMode(MusicGameMode::MULTIPLAYER, true);
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

	// TODO: Ensure that "original_sountrack" is always the first album in the list

	currentSong = 0;

	return !albumList.empty();
}

std::string PlayList_CurrentSong()
{
	if (!playList.empty())
	{
		if (currentSong >= playList.size())
		{
			currentSong = 0;
		}
		auto pSong = playList[currentSong];
		if (pSong)
		{
			return pSong->filename;
		}
	}
	return std::string();
}

std::string PlayList_NextSong()
{
	// If there's a next song in the playlist select it
	if (!playList.empty()
	    && currentSong < (playList.size() - 1))
	{
		currentSong++;
	}
	// Otherwise jump to the start of the playlist
	else
	{
		currentSong = 0;
	}

	return PlayList_CurrentSong();
}
