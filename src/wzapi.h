/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2019  Warzone 2100 Project

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

#ifndef __INCLUDED_WZAPI_H__
#define __INCLUDED_WZAPI_H__

#include "lib/framework/frame.h"
#include "3rdparty/optional/optional.hpp"

using nonstd::optional;
using nonstd::nullopt;

#include <string>
#include <vector>

namespace wzapi
{
#define WZAPI_NO_PARAMS const wzapi::execution_context& context
#define WZAPI_PARAMS(...) const wzapi::execution_context& context, __VA_ARGS__

	class execution_context
	{
	public:
		virtual ~execution_context() { };
	public:
		virtual int player() const = 0;
		virtual void throwError(const char *expr, int line, const char *function) const = 0;
	};

	struct object_id_player_type
	{
		int id;
		int player;
		OBJECT_TYPE type;
	};

	struct structure_id_player
	{
		int id;
		int player;
	};

	struct droid_id_player
	{
		int id = -1;
		int player = -1;
	};

//	struct string_list
//	{
//		const char** strings = nullptr;
//		size_t count = 0;
//	};

	struct va_list_treat_as_strings
	{
		std::vector<std::string> strings;
	};

	struct optional_position
	{
		bool valid;
		int x;
		int y;
	};

//	struct me
//	{
//		int player = -1;
//	};

	struct specified_player
	{
		int player = -1;
	};

	struct no_return_value
	{ };

	struct STRUCTURE_TYPE_or_statsName_string
	{
		STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;
		std::string statsName;
	};

	// retVals
	struct researchResult
	{
		const RESEARCH * psResearch;
		int player;
	};
	struct researchResults
	{
		std::vector<const RESEARCH *> resList;
		int player;
	};

	std::string translate(WZAPI_PARAMS(std::string str));
	int32_t syncRandom(WZAPI_PARAMS(uint32_t limit));
	bool setAlliance(WZAPI_PARAMS(int player1, int player2, bool value));
	no_return_value sendAllianceRequest(WZAPI_PARAMS(int player2));
	bool orderDroid(WZAPI_PARAMS(DROID* psDroid, int order));
	bool orderDroidBuild(WZAPI_PARAMS(DROID* psDroid, int order, std::string statName, int x, int y, optional<float> direction));
	bool setAssemblyPoint(WZAPI_PARAMS(structure_id_player structVal, int x, int y));
	bool setSunPosition(WZAPI_PARAMS(float x, float y, float z));
	bool setSunIntensity(WZAPI_PARAMS(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b));
	bool setWeather(WZAPI_PARAMS(int weather));
	bool setSky(WZAPI_PARAMS(std::string page, float wind, float scale));
	bool cameraSlide(WZAPI_PARAMS(float x, float y));
	bool cameraZoom(WZAPI_PARAMS(float z, float speed));
	bool cameraTrack(WZAPI_PARAMS(optional<DROID *> targetDroid));
	uint32_t addSpotter(WZAPI_PARAMS(int x, int y, int player, int range, bool radar, uint32_t expiry));
	bool removeSpotter(WZAPI_PARAMS(uint32_t id));
	bool syncRequest(WZAPI_PARAMS(int32_t req_id, int32_t x, int32_t y, optional<const BASE_OBJECT *> _psObj, optional<const BASE_OBJECT *> _psObj2));
	bool replaceTexture(WZAPI_PARAMS(std::string oldfile, std::string newfile));
	bool changePlayerColour(WZAPI_PARAMS(int player, int colour));
	bool setHealth(WZAPI_PARAMS(object_id_player_type objVal, int health));
	bool useSafetyTransport(WZAPI_PARAMS(bool flag));
	bool restoreLimboMissionData(WZAPI_NO_PARAMS);
	uint32_t getMultiTechLevel(WZAPI_NO_PARAMS);
	bool setCampaignNumber(WZAPI_PARAMS(int num));

	bool setRevealStatus(WZAPI_PARAMS(bool status));
	bool autoSave(WZAPI_NO_PARAMS);


	bool console(WZAPI_PARAMS(va_list_treat_as_strings strings));
	bool clearConsole(WZAPI_NO_PARAMS);
	bool structureIdle(WZAPI_PARAMS(structure_id_player structVal));
	std::vector<const STRUCTURE *> enumStruct(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const STRUCTURE *> enumStructOffWorld(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking));
	std::vector<const DROID *> enumDroid(WZAPI_PARAMS(optional<int> _player, optional<int> _droidType, optional<int> _looking));
	std::vector<const FEATURE *> enumFeature(WZAPI_PARAMS(int looking, optional<std::string> _statsName));
	std::vector<Position> enumBlips(WZAPI_PARAMS(int player));
	std::vector<const BASE_OBJECT *> enumSelected(WZAPI_NO_PARAMS);
	researchResult getResearch(WZAPI_PARAMS(std::string resName, optional<int> _player));
	researchResults enumResearch(WZAPI_NO_PARAMS);
	std::vector<const BASE_OBJECT *> enumRange(WZAPI_PARAMS(int x, int y, int range, optional<int> _filter, optional<bool> _seen));
}

#endif
