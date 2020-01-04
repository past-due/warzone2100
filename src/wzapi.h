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

namespace wzapi
{
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
		int id = -1;
		int player = -1;
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

	struct string_list
	{
		const char** strings = nullptr;
		size_t count = 0;
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

	const char * translate(WZAPI_PARAMS(std::string str));
	int32_t syncRandom(WZAPI_PARAMS(uint32_t limit));
	bool setAlliance(WZAPI_PARAMS(int player1, int player2, bool value));
	no_return_value sendAllianceRequest(WZAPI_PARAMS(int player2));
	bool orderDroid(WZAPI_PARAMS(DROID* psDroid, int order));
	bool orderDroidBuild(WZAPI_PARAMS(DROID* psDroid, int order, std::string statName, int x, int y, optional<float> direction));
	bool setAssemblyPoint(WZAPI_PARAMS(structure_id_player structVal, int x, int y));
	bool setSunPosition(WZAPI_PARAMS(float x, float y, float z));
}

#endif
