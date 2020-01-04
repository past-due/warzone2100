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
/**
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtScript/QScriptValue>
#include <QtCore/QStringList>
#include <QtCore/QJsonArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QPointer>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic pop // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/fixedpoint.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "qtscriptfuncs.h"
#include "lib/ivis_opengl/tex.h"

#include "action.h"
#include "clparse.h"
#include "combat.h"
#include "console.h"
#include "design.h"
#include "display3d.h"
#include "map.h"
#include "mission.h"
#include "move.h"
#include "order.h"
#include "transporter.h"
#include "message.h"
#include "display3d.h"
#include "intelmap.h"
#include "hci.h"
#include "wrappers.h"
#include "challenge.h"
#include "research.h"
#include "multilimit.h"
#include "multigifts.h"
#include "multimenu.h"
#include "template.h"
#include "lighting.h"
#include "radar.h"
#include "random.h"
#include "frontend.h"
#include "loop.h"
#include "gateway.h"
#include "mapgrid.h"
#include "lighting.h"
#include "atmos.h"
#include "warcam.h"
#include "projectile.h"
#include "component.h"
#include "seqdisp.h"
#include "ai.h"
#include "advvis.h"
#include "loadsave.h"
#include "wzapi.h"
#include "order.h"

/// Assert for scripts that give useful backtraces and other info.
#if defined(SCRIPT_ASSERT)
#undef SCRIPT_ASSERT
#endif
#define SCRIPT_ASSERT(retval, execution_context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			context.throwError(#expr, __LINE__, __FUNCTION__); \
			return retval; } } while (0)

//-- ## _(string)
//--
//-- Mark string for translation.
//--
const char * wzapi::translate(WZAPI_PARAMS(std::string str))
{
	return gettext(str.c_str());
}

//-- ## syncRandom(limit)
//--
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
//--
int32_t wzapi::syncRandom(WZAPI_PARAMS(uint32_t limit))
{
	return gameRand(limit);
}

//-- ## setAlliance(player1, player2, value)
//--
//-- Set alliance status between two players to either true or false. (3.2+ only)
//--
bool wzapi::setAlliance(WZAPI_PARAMS(int player1, int player2, bool value))
{
	if (value)
	{
		formAlliance(player1, player2, true, false, true);
	}
	else
	{
		breakAlliance(player1, player2, true, true);
	}
	return true;
}

//-- ## sendAllianceRequest(player)
//--
//-- Send an alliance request to a player. (3.3+ only)
//--
wzapi::no_return_value wzapi::sendAllianceRequest(WZAPI_PARAMS(int player2))
{
	if (!alliancesFixed(game.alliance))
	{
		requestAlliance(context.player(), player2, true, true);
	}
	return wzapi::no_return_value();
}

//-- ## orderDroid(droid, order)
//--
//-- Give a droid an order to do something. (3.2+ only)
//--
bool wzapi::orderDroid(WZAPI_PARAMS(DROID* psDroid, int order))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(false, context, psDroid, "Droid id not found belonging to player");//, psDroid->id, (int)psDroid->player);
//	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	SCRIPT_ASSERT(false, context, order == DORDER_HOLD || order == DORDER_RTR || order == DORDER_STOP
	              || order == DORDER_RTB || order == DORDER_REARM || order == DORDER_RECYCLE,
	              "Invalid order: %s", getDroidOrderName((DROID_ORDER)order));

	debug(LOG_3D, "WZAPI: droid.id=%d, droid.player=%d, order=%d", psDroid->id, (int)psDroid->player, (int)order);

	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order)
	{
		return true;
	}
	if (order == DORDER_REARM)
	{
		if (STRUCTURE *psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, false))
		{
			orderDroidObj(psDroid, (DROID_ORDER)order, psStruct, ModeQueue);
		}
		else
		{
			orderDroid(psDroid, DORDER_RTB, ModeQueue);
		}
	}
	else
	{
		orderDroid(psDroid, (DROID_ORDER)order, ModeQueue);
	}
	return true;
}

//-- ## orderDroidBuild(droid, order, structure type, x, y[, direction])
//--
//-- Give a droid an order to build something at the given position. Returns true if allowed.
//--
bool wzapi::orderDroidBuild(WZAPI_PARAMS(DROID* psDroid, int order, std::string statName, int x, int y, optional<float> _direction))
{
	// TODO: Change this so instead of taking a DROID *, it takes a droid_id_player struct??
	SCRIPT_ASSERT(false, context, psDroid, "No droid specified");
	debug(LOG_3D, "WZAPI: droid.id=%d, droid.player=%d, order=%d, statName=%s", psDroid->id, (int)psDroid->player, (int)order, statName.c_str());

//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
//	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
//	QString statName = context->argument(2).toString();
	int index = getStructStatFromName(WzString::fromUtf8(statName));
	SCRIPT_ASSERT(false, context, index >= 0, "%s not found", statName.c_str());
	STRUCTURE_STATS	*psStats = &asStructureStats[index];
//	int x = context->argument(3).toInt32();
//	int y = context->argument(4).toInt32();
//	uint16_t direction = 0;
//
	SCRIPT_ASSERT(false, context, order == DORDER_BUILD, "Invalid order");
	SCRIPT_ASSERT(false, context, psStats->id.compare("A0ADemolishStructure") != 0, "Cannot build demolition");
//	if (context->argumentCount() > 5)
//	{
//		direction = DEG(context->argument(5).toNumber());
//	}
	uint16_t uint_direction = 0;
	if (_direction.has_value())
	{
		uint_direction = DEG(_direction.value());
	}

	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return true;
	}
	orderDroidStatsLocDir(psDroid, (DROID_ORDER)order, psStats, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, uint_direction, ModeQueue);
	return true;
}

//-- ## setAssemblyPoint(structure, x, y)
//--
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
//--
bool wzapi::setAssemblyPoint(WZAPI_PARAMS(structure_id_player structVal, int x, int y))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	SCRIPT_ASSERT(false, context, psStruct, "No such structure id %d belonging to player %d", structVal.id, structVal.player);
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(false, context, psStruct->pStructureType->type == REF_FACTORY
	              || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	              || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Structure not a factory");
	setAssemblyPoint(((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint, x, y, structVal.player, true);
	return true;
}

//-- ## setSunPosition(x, y, z)
//--
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
//--
bool wzapi::setSunPosition(WZAPI_PARAMS(float x, float y, float z))
{
//	float x = context->argument(0).toNumber();
//	float y = context->argument(1).toNumber();
//	float z = context->argument(2).toNumber();
	setTheSun(Vector3f(x, y, z));
	return true;
}

//-- ## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)
//--
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
//--
bool wzapi::setSunIntensity(WZAPI_PARAMS(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b))
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	ambient[0] = ambient_r;
	ambient[1] = ambient_g;
	ambient[2] = ambient_b;
	ambient[3] = 1.0f;
	diffuse[0] = diffuse_r;
	diffuse[1] = diffuse_g;
	diffuse[2] = diffuse_b;
	diffuse[3] = 1.0f;
	specular[0] = specular_r;
	specular[1] = specular_g;
	specular[2] = specular_b;
	specular[3] = 1.0f;
	pie_Lighting0(LIGHT_AMBIENT, ambient);
	pie_Lighting0(LIGHT_DIFFUSE, diffuse);
	pie_Lighting0(LIGHT_SPECULAR, specular);
	return true;
}

//-- ## setWeather(weather type)
//--
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
//--
bool wzapi::setWeather(WZAPI_PARAMS(int weather))
{
	SCRIPT_ASSERT(false, context, weather >= 0 && weather <= WT_NONE, "Bad weather type");
	atmosSetWeatherType((WT_CLASS)weather);
	return true;
}

//-- ## setSky(texture file, wind speed, skybox scale)
//--
//-- Change the skybox. (3.2+ only)
//--
bool wzapi::setSky(WZAPI_PARAMS(std::string page, float wind, float scale))
{
//	QString page = context->argument(0).toString();
//	float wind = context->argument(1).toNumber();
//	float scale = context->argument(2).toNumber();
	setSkyBox(page.c_str(), wind, scale);
	return true; // TODO: modify setSkyBox to return bool, success / failure
}

//-- ## cameraSlide(x, y)
//--
//-- Slide the camera over to the given position on the map. (3.2+ only)
//--
bool wzapi::cameraSlide(WZAPI_PARAMS(float x, float y))
{
//	float x = context->argument(0).toNumber();
//	float y = context->argument(1).toNumber();
	requestRadarTrack(x, y);
	return true;
}

//-- ## cameraZoom(z, speed)
//--
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
//--
bool wzapi::cameraZoom(WZAPI_PARAMS(float z, float speed))
{
//	float z = context->argument(0).toNumber();
//	float speed = context->argument(1).toNumber();
	setZoom(speed, z);
	return true;
}

//-- ## cameraTrack(droid)
//--
//-- Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)
//--
bool wzapi::cameraTrack(WZAPI_PARAMS(optional<DROID *> _targetDroid))
{
//	if (context->argument(0).isNull())
	if (!_targetDroid.has_value())
	{
		setWarCamActive(false);
	}
	else
	{
//		QScriptValue droidVal = context->argument(0);
//		int id = droidVal.property("id").toInt32();
//		int player = droidVal.property("player").toInt32();
		DROID *targetDroid = _targetDroid.value(); // DROID *targetDroid = IdToDroid(id, player);
		SCRIPT_ASSERT(false, context, targetDroid, "No such droid id belonging to player");//, id, player);
		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			psDroid->selected = (psDroid == targetDroid); // select only the target droid
		}
		setWarCamActive(true);
	}
	return true;
}

//-- ## addSpotter(x, y, player, range, type, expiry)
//--
//-- Add an invisible viewer at a given position for given player that shows map in given range. ```type```
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)
//--
uint32_t wzapi::addSpotter(WZAPI_PARAMS(int x, int y, int player, int range, bool radar, uint32_t expiry))
{
//	int x = context->argument(0).toInt32();
//	int y = context->argument(1).toInt32();
//	int player = context->argument(2).toInt32();
//	int range = context->argument(3).toInt32();
//	bool radar = context->argument(4).toBool();
//	uint32_t expiry = context->argument(5).toUInt32();
	uint32_t id = ::addSpotter(x, y, player, range, radar, expiry);
	return id;
}

//-- ## removeSpotter(id)
//--
//-- Remove a spotter given its unique ID. (3.2+ only)
//--
bool wzapi::removeSpotter(WZAPI_PARAMS(uint32_t id))
{
//	uint32_t id = context->argument(0).toUInt32();
	return ::removeSpotter(id);
}

//-- ## syncRequest(req_id, x, y[, obj[, obj2]])
//--
//-- Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
//-- Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
//-- take care only to define sync requests that can be validated against cheating. (3.2+ only)
//--
bool wzapi::syncRequest(WZAPI_PARAMS(int32_t req_id, int32_t x, int32_t y, optional<const BASE_OBJECT *> _psObj, optional<const BASE_OBJECT *> _psObj2))
{
//	int32_t req_id = context->argument(0).toInt32();
//	int32_t x = world_coord(context->argument(1).toInt32());
//	int32_t y = world_coord(context->argument(2).toInt32());
	const BASE_OBJECT *psObj = nullptr, *psObj2 = nullptr;
//	if (context->argumentCount() > 3)
	if (_psObj.has_value())
	{
//		QScriptValue objVal = context->argument(3);
//		int oid = objVal.property("id").toInt32();
//		int oplayer = objVal.property("player").toInt32();
//		OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
		psObj = _psObj.value(); // psObj = IdToObject(otype, oid, oplayer);
//		SCRIPT_ASSERT(false, context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
		SCRIPT_ASSERT(false, context, psObj, "No such object id belonging to player");
	}
//	if (context->argumentCount() > 4)
	if (_psObj2.has_value())
	{
//		QScriptValue objVal = context->argument(4);
//		int oid = objVal.property("id").toInt32();
//		int oplayer = objVal.property("player").toInt32();
//		OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
		psObj2 = _psObj2.value(); // psObj2 = IdToObject(otype, oid, oplayer);
//		SCRIPT_ASSERT(false, context, psObj2, "No such object id %d belonging to player %d", oid, oplayer);
		SCRIPT_ASSERT(false, context, psObj2, "No such object id belonging to player");
	}
	sendSyncRequest(req_id, x, y, psObj, psObj2);
	return true;
}

//-- ## replaceTexture(old_filename, new_filename)
//--
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
//--
bool wzapi::replaceTexture(WZAPI_PARAMS(std::string oldfile, std::string newfile))
{
//	QString oldfile = context->argument(0).toString();
//	QString newfile = context->argument(1).toString();
	return replaceTexture(WzString::fromUtf8(oldfile), WzString::fromUtf8(newfile));
}

//-- ## changePlayerColour(player, colour)
//--
//-- Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
//--
bool wzapi::changePlayerColour(WZAPI_PARAMS(int player, int colour))
{
//	int player = context->argument(0).toInt32();
//	int colour = context->argument(1).toInt32();
	return setPlayerColour(player, colour);
}

//-- ## setHealth(object, health)
//--
//-- Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.2.3+ only.)
//--
bool wzapi::setHealth(WZAPI_PARAMS(object_id_player_type objVal, int health))
{
//	QScriptValue objVal = context->argument(0);
//	int health = context->argument(1).toInt32();
	SCRIPT_ASSERT(false, context, health >= 1, "Bad health value %d", health);
	int id = objVal.id; //objVal.property("id").toInt32();
	int player = objVal.player; //objVal.property("player").toInt32();
	OBJECT_TYPE type = objVal.type; //(OBJECT_TYPE)objVal.property("type").toInt32();
	SCRIPT_ASSERT(false, context, type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE, "Bad object type");
	if (type == OBJ_DROID)
	{
		DROID *psDroid = IdToDroid(id, player);
		SCRIPT_ASSERT(false, context, psDroid, "No such droid id %d belonging to player %d", id, player);
		psDroid->body = health * (double)psDroid->originalBody / 100;
	}
	else if (type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = IdToStruct(id, player);
		SCRIPT_ASSERT(false, context, psStruct, "No such structure id %d belonging to player %d", id, player);
		psStruct->body = health * MAX(1, structureBody(psStruct)) / 100;
	}
	else
	{
		FEATURE *psFeat = IdToFeature(id, player);
		SCRIPT_ASSERT(false, context, psFeat, "No such feature id %d belonging to player %d", id, player);
		psFeat->body = health * psFeat->psStats->body / 100;
	}
	return true;
}
