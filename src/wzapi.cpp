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

#include <list>

/// Assert for scripts that give useful backtraces and other info.
#if defined(SCRIPT_ASSERT)
#undef SCRIPT_ASSERT
#endif
#define SCRIPT_ASSERT(retval, execution_context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			context.throwError(#expr, __LINE__, __FUNCTION__); \
			return retval; } } while (0)

#if defined(SCRIPT_ASSERT_PLAYER)
#undef SCRIPT_ASSERT_PLAYER
#endif
#define SCRIPT_ASSERT_PLAYER(retval, _context, _player) \
	SCRIPT_ASSERT(retval, _context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3

//-- ## _(string)
//--
//-- Mark string for translation.
//--
std::string wzapi::translate(WZAPI_PARAMS(std::string str))
{
	return std::string(gettext(str.c_str()));
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
bool wzapi::setAssemblyPoint(WZAPI_PARAMS(STRUCTURE *psStruct, int x, int y))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
//	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	SCRIPT_ASSERT(false, context, psStruct, "No structure provided");
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(false, context, psStruct->pStructureType->type == REF_FACTORY
	              || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	              || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Structure not a factory");
	setAssemblyPoint(((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint, x, y, psStruct->player, true);
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
bool wzapi::cameraZoom(WZAPI_PARAMS(float viewDistance, float speed))
{
//	float viewDistance = context->argument(0).toNumber();
//	float speed = context->argument(1).toNumber();
	animateToViewDistance(viewDistance, speed);
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
bool wzapi::syncRequest(WZAPI_PARAMS(int32_t req_id, int32_t _x, int32_t _y, optional<const BASE_OBJECT *> _psObj, optional<const BASE_OBJECT *> _psObj2))
{
//	int32_t req_id = context->argument(0).toInt32();
	int32_t x = world_coord(_x); //context->argument(1).toInt32());
	int32_t y = world_coord(_y); //context->argument(2).toInt32());
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
bool wzapi::setHealth(WZAPI_PARAMS(object_id_player_type objVal, int health)) MULTIPLAY_SYNCREQUEST_REQUIRED
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

//-- ## useSafetyTransport(flag)
//--
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.3+ only).
//--
bool wzapi::useSafetyTransport(WZAPI_PARAMS(bool flag))
{
//	bool flag = context->argument(0).toBool();
	setDroidsToSafetyFlag(flag);
	return true;
}

//-- ## restoreLimboMissionData()
//--
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.3+ only).
//--
bool wzapi::restoreLimboMissionData(WZAPI_NO_PARAMS)
{
	resetLimboMission();
	return true;
}

//-- ## getMultiTechLevel()
//--
//-- Returns the current multiplayer tech level. (3.3+ only)
//--
uint32_t wzapi::getMultiTechLevel(WZAPI_NO_PARAMS)
{
	return game.techLevel;
}

//-- ## setCampaignNumber(num)
//--
//-- Set the campaign number. (3.3+ only)
//--
bool wzapi::setCampaignNumber(WZAPI_PARAMS(int num))
{
//	int num = context->argument(0).toInt32();
	::setCampaignNumber(num);
	return true;
}

//-- ## setRevealStatus(bool)
//--
//-- Set the fog reveal status. (3.3+ only)
bool wzapi::setRevealStatus(WZAPI_PARAMS(bool status))
{
//	bool status = context->argument(0).toBool();
	::setRevealStatus(status);
	preProcessVisibility();
	return true;
}

//-- ## autoSave()
//--
//-- Perform automatic save
//--
bool wzapi::autoSave(WZAPI_NO_PARAMS)
{
	return ::autoSave();
}






//-- ## console(strings...)
//--
//-- Print text to the player console.
//--
// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
bool wzapi::console(WZAPI_PARAMS(va_list_treat_as_strings strings))
{
	int player = context.player(); //engine->globalObject().property("me").toInt32();
	if (player == selectedPlayer)
	{
		std::string result;
		for (const auto & s : strings.strings)
		{
			if (!result.empty())
			{
				result.append(" ");
			}
//			QString s = context->argument(i).toString();
//			if (context->state() == QScriptContext::ExceptionState)
//			{
//				break;
//			}
			result.append(s);
		}
		//permitNewConsoleMessages(true);
		//setConsolePermanence(true,true);
		addConsoleMessage(result.c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		//permitNewConsoleMessages(false);
	}
	return true;
}

//-- ## clearConsole()
//--
//-- Clear the console. (3.3+ only)
//--
bool wzapi::clearConsole(WZAPI_NO_PARAMS)
{
	flushConsoleMessages();
	return true;
}

//-- ## structureIdle(structure)
//--
//-- Is given structure idle?
//--
bool wzapi::structureIdle(WZAPI_PARAMS(const STRUCTURE *psStruct))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
//	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	SCRIPT_ASSERT(false, context, psStruct, "No structure provided");
	return ::structureIdle(psStruct);
}

std::vector<const STRUCTURE *> _enumStruct_fromList(WZAPI_PARAMS(optional<int> _player, optional<wzapi::STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking), STRUCTURE **psStructLists)
{
	std::vector<const STRUCTURE *> matches;
	int player = -1, looking = -1;
	WzString statsName;
	STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;

//	switch (context->argumentCount())
//	{
//	default:
//	case 3: looking = context->argument(2).toInt32(); // fall-through
//	case 2: val = context->argument(1);
//		if (val.isNumber())
//		{
//			type = (STRUCTURE_TYPE)val.toInt32();
//		}
//		else
//		{
//			statsName = WzString::fromUtf8(val.toString().toUtf8().constData());
//		} // fall-through
//	case 1: player = context->argument(0).toInt32(); break;
//	case 0: player = engine->globalObject().property("me").toInt32();
//	}

	player = (_player.has_value()) ? _player.value() : context.player();

	if (_structureType.has_value())
	{
		type = _structureType.value().type;
		statsName = WzString::fromUtf8(_structureType.value().statsName);
	}
	if (_looking.has_value())
	{
		looking = _looking.value();
	}

	SCRIPT_ASSERT_PLAYER({}, context, player);
//	SCRIPT_ASSERT({}, context, player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = psStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking])
		    && !psStruct->died
		    && (type == NUM_DIFF_BUILDINGS || type == psStruct->pStructureType->type)
		    && (statsName.isEmpty() || statsName.compare(psStruct->pStructureType->id) == 0))
		{
			matches.push_back(psStruct);
		}
	}

	return matches;
}

//-- ## enumStruct([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects. If no parameters given, it will
//-- return all of the structures for the current player. The second parameter
//-- can be either a string with the name of the structure type as defined in
//-- "structures.json", or a stattype as defined in ```Structure```. The
//-- third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
std::vector<const STRUCTURE *> wzapi::enumStruct(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking))
{
	return _enumStruct_fromList(context, _player, _structureType, _looking, apsStructLists);
}

//-- ## enumStructOffWorld([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
//-- If no parameters given, it will return all of the structures for the current player.
//-- The second parameter can be either a string with the name of the structure type as defined
//-- in "structures.json", or a stattype as defined in ```Structure```.
//-- The third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
std::vector<const STRUCTURE *> wzapi::enumStructOffWorld(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking))
{
	return _enumStruct_fromList(context, _player, _structureType, _looking, (mission.apsStructLists));
}

//-- ## enumDroid([player[, droid type[, looking player]]])
//--
//-- Returns an array of droid objects. If no parameters given, it will
//-- return all of the droids for the current player. The second, optional parameter
//-- is the name of the droid type. The third parameter can be used to filter by
//-- visibility - the default is not to filter.
//--
std::vector<const DROID *> wzapi::enumDroid(WZAPI_PARAMS(optional<int> _player, optional<int> _droidType, optional<int> _looking))
{
	std::vector<const DROID *> matches;
	int player = -1, looking = -1;
	DROID_TYPE droidType = DROID_ANY;
	DROID_TYPE droidType2;

//	switch (context->argumentCount())
//	{
//	default:
//	case 3: looking = context->argument(2).toInt32(); // fall-through
//	case 2: droidType = (DROID_TYPE)context->argument(1).toInt32(); // fall-through
//	case 1: player = context->argument(0).toInt32(); break;
//	case 0: player = engine->globalObject().property("me").toInt32();
//	}

//	player = (_player.has_value()) ? _player.value() : context.player();
	if (_player.has_value())
	{
		player = _player.value();
	}
	else
	{
		player = context.player();
	}

	if (_droidType.has_value())
	{
		droidType = (DROID_TYPE)_droidType.value();
	}
	if (_looking.has_value())
	{
		looking = _looking.value();
	}

	switch (droidType) // hide some engine craziness
	{
	case DROID_CONSTRUCT:
		droidType2 = DROID_CYBORG_CONSTRUCT; break;
	case DROID_WEAPON:
		droidType2 = DROID_CYBORG_SUPER; break;
	case DROID_REPAIR:
		droidType2 = DROID_CYBORG_REPAIR; break;
	case DROID_CYBORG:
		droidType2 = DROID_CYBORG_SUPER; break;
	default:
		droidType2 = droidType;
		break;
	}
	SCRIPT_ASSERT_PLAYER({}, context, player);
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if ((looking == -1 || psDroid->visible[looking])
		    && !psDroid->died
		    && (droidType == DROID_ANY || droidType == psDroid->droidType || droidType2 == psDroid->droidType))
		{
			matches.push_back(psDroid);
		}
	}
	return matches;
}

//-- ## enumFeature(player[, name])
//--
//-- Returns an array of all features seen by player of given name, as defined in "features.json".
//-- If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
//--
std::vector<const FEATURE *> wzapi::enumFeature(WZAPI_PARAMS(int looking, optional<std::string> _statsName))
{
	std::vector<const FEATURE *> matches;
//	int looking = context->argument(0).toInt32();
	WzString statsName;
	if (_statsName.has_value())
	{
		statsName = WzString::fromUtf8(_statsName.value());
	}
//	if (context->argumentCount() > 1)
//	{
//		statsName = WzString::fromUtf8(context->argument(1).toString().toUtf8().constData());
//	}
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		if ((looking == -1 || psFeat->visible[looking])
		    && !psFeat->died
		    && (statsName.isEmpty() || statsName.compare(psFeat->psStats->id) == 0))
		{
			matches.push_back(psFeat);
		}
	}
	return matches;
}

//-- ## enumBlips(player)
//--
//-- Return an array containing all the non-transient radar blips that the given player
//-- can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
//-- It does not include units going out of view.
//--
std::vector<Position> wzapi::enumBlips(WZAPI_PARAMS(int player))
{
	std::vector<Position> matches;
//	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	for (BASE_OBJECT *psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
	{
		if (psSensor->visible[player] > 0 && psSensor->visible[player] < UBYTE_MAX)
		{
			matches.push_back(psSensor->pos);
		}
	}
	return matches;
}

//-- ## enumSelected()
//--
//-- Return an array containing all game objects currently selected by the host player. (3.2+ only)
//--
std::vector<const BASE_OBJECT *> wzapi::enumSelected(WZAPI_NO_PARAMS)
{
	std::vector<const BASE_OBJECT *> matches;
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			matches.push_back(psDroid);
		}
	}
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			matches.push_back(psStruct);
		}
	}
	// TODO - also add selected delivery points
	return matches;
}

//-- ## getResearch(research[, player])
//--
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.json". If not found, returns null.
//--
wzapi::researchResult wzapi::getResearch(WZAPI_PARAMS(std::string resName, optional<int> _player))
{
	researchResult result;
	int player = (_player.has_value()) ? _player.value() : context.player();
//	if (context->argumentCount() == 2)
//	{
//		player = context->argument(1).toInt32();
//	}
//	else
//	{
//		player = engine->globalObject().property("me").toInt32();
//	}
//	QString resName = context->argument(0).toString();
	result.psResearch = ::getResearch(resName.c_str());
	result.player = player;
	return result;
//	if (!psResearch)
//	{
//		return QScriptValue::NullValue;
//	}
//	return convResearch(psResearch, engine, player);
}

//-- ## enumResearch()
//--
//-- Returns an array of all research objects that are currently and immediately available for research.
//--
wzapi::researchResults wzapi::enumResearch(WZAPI_NO_PARAMS)
{
	researchResults result;
	int player = context.player(); //engine->globalObject().property("me").toInt32();
	for (int i = 0; i < asResearch.size(); i++)
	{
		RESEARCH *psResearch = &asResearch[i];
		if (!IsResearchCompleted(&asPlayerResList[player][i]) && researchAvailable(i, player, ModeQueue))
		{
			result.resList.push_back(psResearch);
		}
	}
	result.player = player;
	return result;
}

//-- ## enumRange(x, y, range[, filter[, seen]])
//--
//-- Returns an array of game objects seen within range of given position that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. Calling this function is much faster than
//-- iterating over all game objects using other enum functions. (3.2+ only)
//--
std::vector<const BASE_OBJECT *> wzapi::enumRange(WZAPI_PARAMS(int _x, int _y, int _range, optional<int> _filter, optional<bool> _seen))
{
	int player = context.player(); // engine->globalObject().property("me").toInt32();
	int x = world_coord(_x);//context->argument(0).toInt32());
	int y = world_coord(_y);//context->argument(1).toInt32());
	int range = world_coord(_range);//context->argument(2).toInt32());
	int filter = (_filter.has_value()) ? _filter.value() : ALL_PLAYERS;
	bool seen = (_seen.has_value()) ? _seen.value() : true;
//	if (context->argumentCount() > 3)
//	{
//		filter = context->argument(3).toInt32();
//	}
//	if (context->argumentCount() > 4)
//	{
//		seen = context->argument(4).toBool();
//	}
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(x, y, range);
	std::vector<const BASE_OBJECT *> list;
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		const BASE_OBJECT *psObj = *gi;
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((filter >= 0 && psObj->player == filter) || filter == ALL_PLAYERS
			    || (filter == ALLIES && psObj->type != OBJ_FEATURE && aiCheckAlliances(psObj->player, player))
			    || (filter == ENEMIES && psObj->type != OBJ_FEATURE && !aiCheckAlliances(psObj->player, player)))
			{
				list.push_back(psObj);
			}
		}
	}
	return list;
}

//-- ## pursueResearch(lab, research)
//--
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
//--
bool wzapi::pursueResearch(WZAPI_PARAMS(const STRUCTURE *psStruct, string_or_string_list research))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.id; //structVal.property("id").toInt32();
//	int player = structVal.player; //structVal.property("player").toInt32();
//	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(false, context, psStruct, "No structure provided");
	int player = psStruct->player;
//	QScriptValue list = context->argument(1);
	RESEARCH *psResearch = nullptr;  // Dummy initialisation.
	for (const auto& resName : research.strings)
	{
		RESEARCH *psCurrResearch = ::getResearch(resName.c_str());
		SCRIPT_ASSERT(false, context, psCurrResearch, "No such research: %s", resName.c_str());
		PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psCurrResearch->index];
		if (!IsResearchStartedPending(plrRes) && !IsResearchCompleted(plrRes))
		{
			// use this one
			psResearch = psCurrResearch;
			break;
		}
	}
	if (psResearch == nullptr)
	{
		if (research.strings.size() == 1)
		{
			debug(LOG_SCRIPT, "%s has already been researched!", research.strings.front().c_str());
		}
		else
		{
			debug(LOG_SCRIPT, "Exhausted research list -- doing nothing");
		}
		return false;
	}

	SCRIPT_ASSERT(false, context, psStruct->pStructureType->type == REF_RESEARCH, "Not a research lab: %s", objInfo(psStruct));
	RESEARCH_FACILITY *psResLab = (RESEARCH_FACILITY *)psStruct->pFunctionality;
	SCRIPT_ASSERT(false, context, psResLab->psSubject == nullptr, "Research lab not ready");
	// Go down the requirements list for the desired tech
	std::list<RESEARCH *> reslist;
	RESEARCH *cur = psResearch;
	int iterations = 0;  // Only used to assert we're not stuck in the loop.
	while (cur)
	{
		if (researchAvailable(cur->index, player, ModeQueue))
		{
			bool started = false;
			for (int i = 0; i < game.maxPlayers; i++)
			{
				if (i == player || (aiCheckAlliances(player, i) && alliancesSharedResearch(game.alliance)))
				{
					int bits = asPlayerResList[i][cur->index].ResearchStatus;
					started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING)
					          || (bits & RESBITS_PENDING_ONLY) || (bits & RESEARCHED);
				}
			}
			if (!started) // found relevant item on the path?
			{
				sendResearchStatus(const_cast<STRUCTURE *>(psStruct), cur->index, player, true); // FIXME: Remove const_cast
#if defined (DEBUG)
				char sTemp[128];
				snprintf(sTemp, sizeof(sTemp), "player:%d starts topic from script: %s", player, getID(cur));
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				debug(LOG_SCRIPT, "Started research in %d's %s(%d) of %s", player,
				      objInfo(psStruct), psStruct->id, getName(cur));
				return true;
			}
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist.push_back(&asResearch[prev->pPRList[i]]);
		}
		if (!cur && !reslist.empty())
		{
			cur = reslist.front(); // retrieve options from the stack
			reslist.pop_front();
		}
		ASSERT_OR_RETURN(false, ++iterations < asResearch.size() * 100 || !cur, "Possible cyclic dependencies in prerequisites, possibly of research \"%s\".", getName(cur));
	}
	debug(LOG_SCRIPT, "No research topic found for %s(%d)", objInfo(psStruct), psStruct->id);
	return false; // none found
}

//-- ## findResearch(research, [player])
//--
//-- Return list of research items remaining to be researched for the given research item. (3.2+ only)
//-- (Optional second argument 3.2.3+ only)
//--
wzapi::researchResults wzapi::findResearch(WZAPI_PARAMS(std::string resName, optional<int> _player))
{
//	QString resName = context->argument(0).toString();
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);

	researchResults result;
	result.player = player;

	RESEARCH *psTarget = ::getResearch(resName.c_str());
	SCRIPT_ASSERT({}, context, psTarget, "No such research: %s", resName.c_str());
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psTarget->index];
	if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
	{
		return result; // return empty array
	}
	debug(LOG_SCRIPT, "Find reqs for %s for player %d", resName.c_str(), player);
	// Go down the requirements list for the desired tech
	std::list<RESEARCH *> reslist;
	RESEARCH *cur = psTarget;
	while (cur)
	{
		if (!(asPlayerResList[player][cur->index].ResearchStatus & RESEARCHED))
		{
			debug(LOG_SCRIPT, "Added research in %d's %s for %s", player, getID(cur), getID(psTarget));
			result.resList.push_back(cur);
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist.push_back(&asResearch[prev->pPRList[i]]);
		}
		if (!cur && !reslist.empty())
		{
			// retrieve options from the stack
			cur = reslist.front();
			reslist.pop_front();
		}
	}
	return result;
}

//-- ## distBetweenTwoPoints(x1, y1, x2, y2)
//--
//-- Return distance between two points.
//--
int32_t wzapi::distBetweenTwoPoints(WZAPI_PARAMS(int32_t x1, int32_t y1, int32_t x2, int32_t y2))
{
	return iHypot(x1 - x2, y1 - y2);
}

//-- ## orderDroidLoc(droid, order, x, y)
//--
//-- Give a droid an order to do something at the given location.
//--
bool wzapi::orderDroidLoc(WZAPI_PARAMS(DROID *psDroid, int order_, int x, int y))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	QScriptValue orderVal = context->argument(1);
//	int x = context->argument(2).toInt32();
//	int y = context->argument(3).toInt32();
	DROID_ORDER order = (DROID_ORDER)order_;
	SCRIPT_ASSERT(false, context, validOrderForLoc(order), "Invalid location based order: %s", getDroidOrderName(order));
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(false, context, psDroid, "Droid not provided");
	SCRIPT_ASSERT(false, context, tileOnMap(x, y), "Outside map bounds (%d, %d)", x, y);
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return true;
	}
	::orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
	return true;
}

//-- ## playerPower(player)
//--
//-- Return amount of power held by the given player.
//--
int32_t wzapi::playerPower(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(-1, context, player);
	return getPower(player);
}

//-- ## queuedPower(player)
//--
//-- Return amount of power queued up for production by the given player. (3.2+ only)
//--
int wzapi::queuedPower(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(-1, context, player);
	return getQueuedPower(player);
}

//-- ## isStructureAvailable(structure type[, player])
//--
//-- Returns true if given structure can be built. It checks both research and unit limits.
//--
bool wzapi::isStructureAvailable(WZAPI_PARAMS(std::string structName, optional<int> _player))
{
	int index = getStructStatFromName(WzString::fromUtf8(structName));
	SCRIPT_ASSERT(false, context, index >= 0, "%s not found", structName.c_str());
	int player = (_player.has_value()) ? _player.value() : context.player();

	int status = apStructTypeLists[player][index];
	return ((status == AVAILABLE || status == REDUNDANT)
	                    && asStructureStats[index].curCount[player] < asStructureStats[index].upgrade[player].limit);
}

// additional structure check
static bool structDoubleCheck(BASE_STATS *psStat, UDWORD xx, UDWORD yy, SDWORD maxBlockingTiles)
{
	UDWORD		x, y, xTL, yTL, xBR, yBR;
	UBYTE		count = 0;
	STRUCTURE_STATS	*psBuilding = (STRUCTURE_STATS *)psStat;

	xTL = xx - 1;
	yTL = yy - 1;
	xBR = (xx + psBuilding->baseWidth);
	yBR = (yy + psBuilding->baseBreadth);

	// check against building in a gateway, as this can seriously block AI passages
	for (auto psGate : gwGetGateways())
	{
		for (x = xx; x <= xBR; x++)
		{
			for (y = yy; y <= yBR; y++)
			{
				if ((x >= psGate->x1 && x <= psGate->x2) && (y >= psGate->y1 && y <= psGate->y2))
				{
					return false;
				}
			}
		}
	}

	// can you get past it?
	y = yTL;	// top
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	y = yBR;	// bottom
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xTL;	// left
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xBR;	// right
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	//make sure this location is not blocked from too many sides
	if ((count <= maxBlockingTiles) || (maxBlockingTiles == -1))
	{
		return true;
	}
	return false;
}

//-- ## pickStructLocation(droid, structure type, x, y)
//--
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
//--
optional<wzapi::position_in_map_coords> wzapi::pickStructLocation(WZAPI_PARAMS(DROID *psDroid, std::string statName, int startX, int startY, optional<int> _maxBlockingTiles))
{
//	QScriptValue droidVal = context->argument(0);
//	const int id = droidVal.property("id").toInt32();
//	const int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
//	QString statName = context->argument(1).toString();
	const int player = psDroid->player;
	int index = getStructStatFromName(WzString::fromUtf8(statName));
	SCRIPT_ASSERT({}, context, index >= 0, "%s not found", statName.c_str());
	STRUCTURE_STATS	*psStat = &asStructureStats[index];

	int numIterations = 30;
	bool found = false;
	int incX, incY, x, y;
	int maxBlockingTiles = 0;

	SCRIPT_ASSERT({}, context, psDroid, "No droid provided");
	SCRIPT_ASSERT({}, context, psStat, "No such stat found: %s", statName.c_str());
	SCRIPT_ASSERT_PLAYER({}, context, player);
	SCRIPT_ASSERT({}, context, startX >= 0 && startX < mapWidth && startY >= 0 && startY < mapHeight, "Bad position (%d, %d)", startX, startY);

	if (_maxBlockingTiles.has_value())
	{
		maxBlockingTiles = _maxBlockingTiles.value();
	}

	x = startX;
	y = startY;

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));

	// save a lot of typing... checks whether a position is valid
#define LOC_OK(_x, _y) (tileOnMap(_x, _y) && \
                        (!psDroid || fpathCheck(psDroid->pos, Vector3i(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
                        && validLocation(psStat, world_coord(Vector2i(_x, _y)) + offset, 0, player, false) && structDoubleCheck(psStat, _x, _y, maxBlockingTiles))

	// first try the original location
	if (LOC_OK(startX, startY))
	{
		found = true;
	}

	// try some locations nearby
	for (incX = 1, incY = 1; incX < numIterations && !found; incX++, incY++)
	{
		y = startY - incY;	// top
		for (x = startX - incX; x < startX + incX; x++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX + incX;	// right
		for (y = startY - incY; y < startY + incY; y++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		y = startY + incY;	// bottom
		for (x = startX + incX; x > startX - incX; x--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX - incX;	// left
		for (y = startY + incY; y > startY - incY; y--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
	}

endstructloc:
	if (found)
	{
//		QScriptValue retval = engine->newObject();
//		retval.setProperty("x", x + map_coord(offset.x), QScriptValue::ReadOnly);
//		retval.setProperty("y", y + map_coord(offset.y), QScriptValue::ReadOnly);
//		retval.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
//		return retval;
		return optional<wzapi::position_in_map_coords>({x + map_coord(offset.x), y + map_coord(offset.y)});
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", getName(psStat));
	}
	return {};
}

//-- ## droidCanReach(droid, x, y)
//--
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
//--
bool wzapi::droidCanReach(WZAPI_PARAMS(const DROID *psDroid, int x, int y))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(false, context, psDroid, "Droid not provided");
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	return fpathCheck(psDroid->pos, Vector3i(world_coord(x), world_coord(y), 0), psPropStats->propulsionType);
}

//-- ## propulsionCanReach(propulsion, x1, y1, x2, y2)
//--
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
//--
bool wzapi::propulsionCanReach(WZAPI_PARAMS(std::string propulsionName, int x1, int y1, int x2, int y2))
{
//	QScriptValue propulsionValue = context->argument(0);
	int propulsion = getCompFromName(COMP_PROPULSION, WzString::fromUtf8(propulsionName));
	SCRIPT_ASSERT(false, context, propulsion > 0, "No such propulsion: %s", propulsionName.c_str());
	const PROPULSION_STATS *psPropStats = asPropulsionStats + propulsion;
	return fpathCheck(Vector3i(world_coord(x1), world_coord(y1), 0), Vector3i(world_coord(x2), world_coord(y2), 0), psPropStats->propulsionType);
}

//-- ## terrainType(x, y)
//--
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
//--
int wzapi::terrainType(WZAPI_PARAMS(int x, int y))
{
	return ::terrainType(mapTile(x, y));
}

//-- ## orderDroidObj(droid, order, object)
//--
//-- Give a droid an order to do something to something.
//--
bool wzapi::orderDroidObj(WZAPI_PARAMS(DROID *psDroid, int _order, BASE_OBJECT *psObj))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(false, context, psDroid, "Droid not provided");
	DROID_ORDER order = (DROID_ORDER)_order; //context->argument(1).toInt32();
//	QScriptValue objVal = context->argument(2);
//	int oid = objVal.property("id").toInt32();
//	int oplayer = objVal.property("player").toInt32();
//	OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
//	BASE_OBJECT *psObj = IdToObject(otype, oid, oplayer);
	SCRIPT_ASSERT(false, context, psObj, "Object not provided");
	SCRIPT_ASSERT(false, context, validOrderForObj(order), "Invalid order: %s", getDroidOrderName(order));
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->order.psObj == psObj)
	{
		return true;
	}
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return true;
}

static int get_first_available_component(int player, int capacity, const wzapi::string_or_string_list& list, COMPONENT_TYPE type, bool strict)
{
	for (const auto& compName : list.strings)
	{
		int result = getCompFromName(type, WzString::fromUtf8(compName));
		if (result >= 0)
		{
			int status = apCompLists[player][type][result];
			if (((status == AVAILABLE || status == REDUNDANT) || !strict)
				&& (type != COMP_BODY || asBodyStats[result].size <= capacity))
			{
				return result; // found one!
			}
		}
		if (result < 0)
		{
			debug(LOG_ERROR, "No such component: %s", compName.c_str());
		}
	}
	return -1; // no available component found in list
}

static DROID_TEMPLATE *makeTemplate(int player, const std::string &templName, const wzapi::string_or_string_list& _body, const wzapi::string_or_string_list& _propulsion, const wzapi::va_list<wzapi::string_or_string_list>& _turrets, int capacity, bool strict)
{
//	const int firstTurret = paramstart + 4; // index position of first turret parameter
	DROID_TEMPLATE *psTemplate = new DROID_TEMPLATE;
	int numTurrets = _turrets.va_list.size(); // anything beyond first six parameters, are turrets
	int result;

	memset(psTemplate->asParts, 0, sizeof(psTemplate->asParts)); // reset to defaults
	memset(psTemplate->asWeaps, 0, sizeof(psTemplate->asWeaps));
	int body = get_first_available_component(player, capacity, _body, COMP_BODY, strict);
	if (body < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but body types all unavailable",
		      templName.c_str());
		delete psTemplate;
		return nullptr; // no component available
	}
	int prop = get_first_available_component(player, capacity, _propulsion, COMP_PROPULSION, strict);
	if (prop < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but propulsion types all unavailable",
		      templName.c_str());
		delete psTemplate;
		return nullptr; // no component available
	}
	psTemplate->asParts[COMP_BODY] = body;
	psTemplate->asParts[COMP_PROPULSION] = prop;

	psTemplate->numWeaps = 0;
	numTurrets = MIN(numTurrets, asBodyStats[body].weaponSlots); // Restrict max no. turrets
	if (asBodyStats[body].droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = asBodyStats[body].droidTypeOverride; // set droidType based on body
	}
	// Find first turret component type (assume every component in list is same type)
	std::string compName = _turrets.va_list[0].strings[0];
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(compName.c_str()));
	if (psComp == nullptr)
	{
		debug(LOG_ERROR, "Wanted to build %s but %s does not exist", templName.c_str(), compName.c_str());
		delete psTemplate;
		return nullptr;
	}
	if (psComp->droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = psComp->droidTypeOverride; // set droidType based on component
	}
	if (psComp->compType == COMP_WEAPON)
	{
		for (int i = 0; i < numTurrets; i++) // may be multi-weapon
		{
			result = get_first_available_component(player, SIZE_NUM, _turrets.va_list[i], COMP_WEAPON, strict);
			if (result < 0)
			{
				debug(LOG_SCRIPT, "Wanted to build %s but no weapon available", templName.c_str());
				delete psTemplate;
				return nullptr;
			}
			psTemplate->asWeaps[i] = result;
			psTemplate->numWeaps++;
		}
	}
	else
	{
		if (psComp->compType == COMP_BRAIN)
		{
			psTemplate->numWeaps = 1; // hack, necessary to pass intValidTemplate
		}
		result = get_first_available_component(player, SIZE_NUM, _turrets.va_list[0], psComp->compType, strict);
		if (result < 0)
		{
			debug(LOG_SCRIPT, "Wanted to build %s but turret unavailable", templName.c_str());
			delete psTemplate;
			return nullptr;
		}
		psTemplate->asParts[psComp->compType] = result;
	}
	bool valid = intValidTemplate(psTemplate, templName.c_str(), true, player);
	if (valid)
	{
		return psTemplate;
	}
	else
	{
		delete psTemplate;
		debug(LOG_ERROR, "Invalid template %s", templName.c_str());
		return nullptr;
	}
}

//-- ## buildDroid(factory, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Start factory production of new droid with the given name, body, propulsion and turrets.
//-- The reserved parameter should be passed **null** for now. The components can be
//-- passed as ordinary strings, or as a list of strings. If passed as a list, the first available
//-- component in the list will be used. The second reserved parameter used to be a droid type.
//-- It is now unused and in 3.2+ should be passed "", while in 3.1 it should be the
//-- droid type to be built. Returns a boolean that is true if production was started.
//--
bool wzapi::buildDroid(WZAPI_PARAMS(STRUCTURE *psFactory, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = psFactory; //IdToStruct(id, player);
	SCRIPT_ASSERT(false, context, psStruct, "No structure provided");
	SCRIPT_ASSERT(false, context, (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	                        || psStruct->pStructureType->type == REF_VTOL_FACTORY), "Structure %s is not a factory", objInfo(psStruct));
	int player = psStruct->player;
//	QString templName = context->argument(1).toString();
	const int capacity = psStruct->capacity; // body size limit
	DROID_TEMPLATE *psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, capacity, true);
	if (psTemplate)
	{
		SCRIPT_ASSERT(false, context, validTemplateForFactory(psTemplate, psStruct, true),
		              "Invalid template %s for factory %s",
		              getName(psTemplate), getName(psStruct->pStructureType));
		// Delete similar template from existing list before adding this one
		for (auto t : apsTemplateList)
		{
			if (t->name.compare(psTemplate->name) == 0)
			{
				debug(LOG_SCRIPT, "deleting %s for player %d", getName(t), player);
				deleteTemplateFromProduction(t, player, ModeQueue); // duplicate? done below?
				break;
			}
		}
		// Add to list
		debug(LOG_SCRIPT, "adding template %s for player %d", getName(psTemplate), player);
		psTemplate->multiPlayerID = generateNewObjectId();
		addTemplate(player, psTemplate);
		if (!structSetManufacture(psStruct, psTemplate, ModeQueue))
		{
			debug(LOG_ERROR, "Could not produce template %s in %s", getName(psTemplate), objInfo(psStruct));
			return false;
		}
	}
	return (psTemplate != nullptr);
}

//-- ## addDroid(player, x, y, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Create and place a droid at the given x, y position as belonging to the given player, built with
//-- the given components. Currently does not support placing droids in multiplayer, doing so will
//-- cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
//-- reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
//-- pass -1 as both x and y.
//--
const DROID* wzapi::addDroid(WZAPI_PARAMS(int player, int x, int y, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets)) MUTLIPLAY_UNSAFE
{
//	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(nullptr, context, player);
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
//	QString templName = context->argument(3).toString();
	bool onMission = (x == -1) && (y == -1);
	SCRIPT_ASSERT(nullptr, context, (onMission || (x >= 0 && y >= 0)), "Invalid coordinates (%d, %d) for droid", x, y);
	DROID_TEMPLATE *psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, SIZE_NUM, false);
	if (psTemplate)
	{
		DROID *psDroid = nullptr;
		bool oldMulti = bMultiMessages;
		bMultiMessages = false; // ugh, fixme
		if (onMission)
		{
			psDroid = ::buildMissionDroid(psTemplate, 128, 128, player);
			if (psDroid)
			{
				debug(LOG_LIFE, "Created mission-list droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.c_str());
			}
		}
		else
		{
			psDroid = ::buildDroid(psTemplate, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, player, onMission, nullptr);
			if (psDroid)
			{
				addDroid(psDroid, apsDroidLists);
				debug(LOG_LIFE, "Created droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.c_str());
			}
		}
		bMultiMessages = oldMulti; // ugh
		delete psTemplate;
		return psDroid;
	}
	return nullptr;
}

//-- ## makeTemplate(player, name, body, propulsion, reserved, turrets...)
//--
//-- Create a template (virtual droid) with the given components. Can be useful for calculating the cost
//-- of droids before putting them into production, for instance. Will fail and return null if template
//-- could not possibly be built using current research. (3.2+ only)
//--
std::unique_ptr<const DROID_TEMPLATE> wzapi::makeTemplate(WZAPI_PARAMS(int player, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, va_list<string_or_string_list> turrets))
{
//	int player = context->argument(0).toInt32();
//	QString templName = context->argument(1).toString();
	DROID_TEMPLATE *psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, SIZE_NUM, true);
	return std::unique_ptr<const DROID_TEMPLATE>(psTemplate);
}

//-- ## addDroidToTransporter(transporter, droid)
//--
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
//--
bool wzapi::addDroidToTransporter(WZAPI_PARAMS(droid_id_player transporter, droid_id_player droid))
{
//	QScriptValue transporterVal = context->argument(0);
	int transporterId = transporter.id; //transporterVal.property("id").toInt32();
	int transporterPlayer = transporter.player; //transporterVal.property("player").toInt32();
	DROID *psTransporter = IdToMissionDroid(transporterId, transporterPlayer);
	SCRIPT_ASSERT(false, context, psTransporter, "No such transporter id %d belonging to player %d", transporterId, transporterPlayer);
	SCRIPT_ASSERT(false, context, isTransporter(psTransporter), "Droid id %d belonging to player %d is not a transporter", transporterId, transporterPlayer);
//	QScriptValue droidVal = context->argument(1);
	int droidId = droid.id; //droidVal.property("id").toInt32();
	int droidPlayer = droid.player; //droidVal.property("player").toInt32();
	DROID *psDroid = IdToMissionDroid(droidId, droidPlayer);
	SCRIPT_ASSERT(false, context, psDroid, "No such droid id %d belonging to player %d", droidId, droidPlayer);
	SCRIPT_ASSERT(false, context, checkTransporterSpace(psTransporter, psDroid), "Not enough room in transporter %d for droid %d", transporterId, droidId);
	bool removeSuccessful = droidRemove(psDroid, mission.apsDroidLists);
	SCRIPT_ASSERT(false, context, removeSuccessful, "Could not remove droid id %d from mission list", droidId);
	psTransporter->psGroup->add(psDroid);
	return true;
}

//-- ## addFeature(name, x, y)
//--
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
//--
const FEATURE * wzapi::addFeature(WZAPI_PARAMS(std::string featName, int x, int y)) MUTLIPLAY_UNSAFE
{
//	QString featName = context->argument(0).toString();
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
	int feature = getFeatureStatFromName(WzString::fromUtf8(featName));
	FEATURE_STATS *psStats = &asFeatureStats[feature];
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		SCRIPT_ASSERT(nullptr, context, map_coord(psFeat->pos.x) != x || map_coord(psFeat->pos.y) != y,
		              "Building feature on tile already occupied");
	}
	FEATURE *psFeature = buildFeature(psStats, world_coord(x), world_coord(y), false);
	return psFeature;
}

//-- ## componentAvailable([component type,] component name)
//--
//-- Checks whether a given component is available to the current player. The first argument is
//-- optional and deprecated.
//--
bool wzapi::componentAvailable(WZAPI_PARAMS(std::string arg1, optional<std::string> arg2))
{
	int player = context.player(); //engine->globalObject().property("me").toInt32();
//	QString id = (context->argumentCount() == 1) ? context->argument(0).toString() : context->argument(1).toString();
	std::string &id = (arg2.has_value()) ? arg2.value() : arg1;
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(id.c_str()));
	SCRIPT_ASSERT(false, context, psComp, "No such component: %s", id.c_str());
	int status = apCompLists[player][psComp->compType][psComp->index];
	return (status == AVAILABLE || status == REDUNDANT);
}

//-- ## isVTOL(droid)
//--
//-- Returns true if given droid is a VTOL (not including transports).
//--
bool wzapi::isVTOL(WZAPI_PARAMS(const DROID *psDroid))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(false, context, psDroid, "No droid provided");
	return isVtolDroid(psDroid);
}

//-- ## safeDest(player, x, y)
//--
//-- Returns true if given player is safe from hostile fire at the given location, to
//-- the best of that player's map knowledge. Does not work in campaign at the moment.
//--
bool wzapi::safeDest(WZAPI_PARAMS(int player, int x, int y))
{
//	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(false, context, player);
//	int x = context->argument(1).toInt32();
//	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(false, context, tileOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	return (!(auxTile(x, y, player) & AUXBITS_DANGER));
}

//-- ## activateStructure(structure, [target[, ability]])
//--
//-- Activate a special ability on a structure. Currently only works on the lassat.
//-- The lassat needs a target.
//--
bool wzapi::activateStructure(WZAPI_PARAMS(STRUCTURE *psStruct, optional<BASE_OBJECT *> _psTarget))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
//	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(false, context, psStruct, "No structure provided");
	int player = psStruct->player;
	// ... and then do nothing with psStruct yet
//	QScriptValue objVal = context->argument(1);
//	int oid = objVal.property("id").toInt32();
//	int oplayer = objVal.property("player").toInt32();
//	OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
//	BASE_OBJECT *psObj = IdToObject(otype, oid, oplayer);
	BASE_OBJECT *psTarget = (_psTarget.has_value()) ? _psTarget.value() : nullptr;
	SCRIPT_ASSERT(false, context, psTarget, "No target provided");
	orderStructureObj(player, psTarget);
	return true;
}

//-- ## chat(target player, message)
//--
//-- Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::chat(WZAPI_PARAMS(int target, std::string message))
{
	int player = context.player(); //engine->globalObject().property("me").toInt32();
//	int target = context->argument(0).toInt32();
//	QString message = context->argument(1).toString();
	SCRIPT_ASSERT(false, context, target >= 0 || target == ALL_PLAYERS || target == ALLIES, "Message to invalid player %d", target);
	if (target == ALL_PLAYERS) // all
	{
		return sendTextMessage(message.c_str(), true, player);
	}
	else if (target == ALLIES) // allies
	{
		return sendTextMessage((std::string(". ") + message).c_str(), false, player);
	}
	else // specific player
	{
		WzString tmp = WzString::number(NetPlay.players[target].position) + WzString::fromUtf8(message);
		return sendTextMessage(tmp.toUtf8().c_str(), false, player);
	}
}

//-- ## addBeacon(x, y, target player[, message])
//--
//-- Send a beacon message to target player. Target may also be ```ALLIES```.
//-- Message is currently unused. Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::addBeacon(WZAPI_PARAMS(int _x, int _y, int target, optional<std::string> _message))
{
	int x = world_coord(_x); //context->argument(0).toInt32());
	int y = world_coord(_y); //context->argument(1).toInt32());
//	int target = context->argument(2).toInt32();
//	QString message = context->argument(3).toString();
	std::string message;
	if (_message.has_value())
	{
		message = _message.value();
	}
	int me = context.player(); //engine->globalObject().property("me").toInt32();
	SCRIPT_ASSERT(false, context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i != me && (i == target || (target == ALLIES && aiCheckAlliances(i, me))))
		{
			debug(LOG_MSG, "adding script beacon to %d from %d", i, me);
			sendBeaconToPlayer(x, y, i, me, message.c_str());
		}
	}
	return true;
}

//-- ## removeBeacon(target player)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::removeBeacon(WZAPI_PARAMS(int target))
{
	int me = context.player(); //engine->globalObject().property("me").toInt32();
//	int target = context->argument(0).toInt32();
	SCRIPT_ASSERT(false, context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == target || (target == ALLIES && aiCheckAlliances(i, me)))
		{
			MESSAGE *psMessage = findBeaconMsg(i, me);
			if (psMessage)
			{
				removeMessage(psMessage, i);
				triggerEventBeaconRemoved(me, i);
			}
		}
	}
//	jsDebugMessageUpdate();
	return true;
}

//-- ## getDroidProduction(factory)
//--
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere. (3.2+ only)
//--
std::unique_ptr<const DROID> wzapi::getDroidProduction(WZAPI_PARAMS(STRUCTURE *_psFactory))
{
//	QScriptValue structVal = context->argument(0);
//	int id = structVal.property("id").toInt32();
//	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = _psFactory; //IdToStruct(id, player);
	SCRIPT_ASSERT(nullptr, context, psStruct, "No structure provided");
	int player = psStruct->player;
	FACTORY *psFactory = &psStruct->pFunctionality->factory;
	DROID_TEMPLATE *psTemp = psFactory->psSubject;
	if (!psTemp)
	{
		return nullptr;
	}
	DROID *psDroid = new DROID(0, player);
	psDroid->pos = psStruct->pos;
	psDroid->rot = psStruct->rot;
	psDroid->experience = 0;
	droidSetName(psDroid, getName(psTemp));
	droidSetBits(psTemp, psDroid);
	psDroid->weight = calcDroidWeight(psTemp);
	psDroid->baseSpeed = calcDroidBaseSpeed(psTemp, psDroid->weight, player);
	return std::unique_ptr<const DROID>(psDroid);
}

//-- ## getDroidLimit([player[, unit type]])
//--
//-- Return maximum number of droids that this player can produce. This limit is usually
//-- fixed throughout a game and the same for all players. If no arguments are passed,
//-- returns general unit limit for the current player. If a second, unit type argument
//-- is passed, the limit for this unit type is returned, which may be different from
//-- the general unit limit (eg for commanders and construction droids). (3.2+ only)
//--
int wzapi::getDroidLimit(WZAPI_PARAMS(optional<int> _player, optional<int> _unitType))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(false, context, player);
	if (_unitType.has_value())
	{
		DROID_TYPE type = (DROID_TYPE)_unitType.value();
		if (type == DROID_COMMAND)
		{
			return getMaxCommanders(player);
		}
		else if (type == DROID_CONSTRUCT)
		{
			return getMaxConstructors(player);
		}
		// else return general unit limit
	}
	return getMaxDroids(player);
}

//-- ## getExperienceModifier(player)
//--
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
//--
int wzapi::getExperienceModifier(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	return getExpGain(player);
}

//-- ## setDroidLimit(player, value[, droid type])
//--
//-- Set the maximum number of droids that this player can produce. If a third
//-- parameter is added, this is the droid type to limit. It can be DROID_ANY
//-- for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
//-- for commanders. (3.2+ only)
//--
bool wzapi::setDroidLimit(WZAPI_PARAMS(int player, int value, optional<int> _droidType))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
//	int player = context->argument(0).toInt32();
//	int value = context->argument(1).toInt32();
	DROID_TYPE type = DROID_ANY;
	if (_droidType.has_value())
	{
		type = (DROID_TYPE)_droidType.value();
	}
	switch (type)
	{
	case DROID_CONSTRUCT:
		setMaxConstructors(player, value);
		break;
	case DROID_COMMAND:
		setMaxCommanders(player, value);
		break;
	default:
	case DROID_ANY:
		setMaxDroids(player, value);
		break;
	}
	return true;
}

//-- ## setExperienceModifier(player, percent)
//--
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
//--
bool wzapi::setExperienceModifier(WZAPI_PARAMS(int player, int percent))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	setExpGain(player, percent);
	return true;
}

//-- ## enumCargo(transport droid)
//--
//-- Returns an array of droid objects inside given transport. (3.2+ only)
//--
std::vector<const DROID *> wzapi::enumCargo(WZAPI_PARAMS(DROID *psDroid))
{
//	QScriptValue droidVal = context->argument(0);
//	int id = droidVal.property("id").toInt32();
//	int player = droidVal.property("player").toInt32();
//	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT({}, context, psDroid, "No droid provided");
	SCRIPT_ASSERT({}, context, isTransporter(psDroid), "Wrong droid type (expecting: transporter)");
	std::vector<const DROID *> result;
	result.reserve(psDroid->psGroup->getNumMembers());
//	QScriptValue result = engine->newArray(psDroid->psGroup->getNumMembers());
	int i = 0;
	for (DROID *psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext, i++)
	{
		if (psDroid != psCurr)
		{
			result.push_back(psCurr);
		}
	}
	return result;
}

// MARK: - Functions that operate on the current player only

//-- ## centreView(x, y)
//--
//-- Center the player's camera at the given position.
//--
bool wzapi::centreView(WZAPI_PARAMS(int x, int y))
{
	setViewPos(x, y, false);
	return true;
}

//-- ## playSound(sound[, x, y, z])
//--
//-- Play a sound, optionally at a location.
//--
bool wzapi::playSound(WZAPI_PARAMS(std::string sound, optional<int> _x, optional<int> _y, optional<int> _z))
{
	int player = context.player(); //engine->globalObject().property("me").toInt32();
	if (player != selectedPlayer)
	{
		return false;
	}
	int soundID = audio_GetTrackID(sound.c_str());
	if (soundID == SAMPLE_NOT_FOUND)
	{
		soundID = audio_SetTrackVals(sound.c_str(), false, 100, 1800);
	}
	if (_x.has_value() || _y.has_value() || _z.has_value())
	{
		SCRIPT_ASSERT(false, context, _x.has_value() && _y.has_value() && _z.has_value(), "If specifying an optional location, x, y, and z must be provided");
		int x = world_coord(_x.value());
		int y = world_coord(_y.value());
		int z = world_coord(_z.value());
		audio_QueueTrackPos(soundID, x, y, z);
	}
	else
	{
		audio_QueueTrack(soundID);
	}
	return true;
}

//-- ## gameOverMessage(won, showBackDrop, showOutro)
//--
//-- End game in victory or defeat.
//--
bool wzapi::gameOverMessage(WZAPI_PARAMS(bool gameWon, optional<bool> _showBackDrop, optional<bool> _showOutro))
{
	int player = context.player(); //engine->globalObject().property("me").toInt32();
	const MESSAGE_TYPE msgType = MSG_MISSION;	// always
	bool showOutro = false;
	bool showBackDrop = true;
	if (_showBackDrop.has_value())
	{
		showBackDrop = _showBackDrop.value();
	}
	if (_showOutro.has_value())
	{
		showOutro = _showOutro.value();
	}
	VIEWDATA *psViewData;
	if (gameWon)
	{
		//Quick hack to stop assert when trying to play outro in campaign.
		psViewData = !bMultiPlayer && showOutro ? getViewData("END") : getViewData("WIN");
		addConsoleMessage(_("YOU ARE VICTORIOUS!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		psViewData = getViewData("END");	// FIXME: rename to FAILED|LOST ?
		addConsoleMessage(_("YOU WERE DEFEATED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	ASSERT(psViewData, "Viewdata not found");
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (!bMultiPlayer && psMessage)
	{
		//we need to set this here so the VIDEO_QUIT callback is not called
		setScriptWinLoseVideo(gameWon ? PLAY_WIN : PLAY_LOSE);
		seq_ClearSeqList();
		if (gameWon && showOutro)
		{
			showBackDrop = false;
			seq_AddSeqToList("outro.ogg", nullptr, "outro.txa", false);
			seq_StartNextFullScreenVideo();
		}
		else
		{
			//set the data
			psMessage->pViewData = psViewData;
			displayImmediateMessage(psMessage);
			stopReticuleButtonFlash(IDRET_INTEL_MAP);
		}
	}
//	jsDebugMessageUpdate();
	displayGameOver(gameWon, showBackDrop);
	if (challengeActive)
	{
		updateChallenge(gameWon);
	}
	if (autogame_enabled())
	{
		debug(LOG_WARNING, "Autogame completed successfully!");
		exit(0);
	}
	return true;
}

// MARK: - Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)

//-- ## setStructureLimits(structure type, limit[, player])
//--
//-- Set build limits for a structure.
//--
bool wzapi::setStructureLimits(WZAPI_PARAMS(std::string building, int limit, optional<int> _player))
{
//	QString building = context->argument(0).toString();
//	int limit = context->argument(1).toInt32();
	int structInc = getStructStatFromName(WzString::fromUtf8(building));
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(false, context, player);
	SCRIPT_ASSERT(false, context, limit < LOTS_OF && limit >= 0, "Invalid limit");
	SCRIPT_ASSERT(false, context, structInc < numStructureStats && structInc >= 0, "Invalid structure");

	asStructureStats[structInc].upgrade[player].limit = limit;

	return true;
}

//-- ## applyLimitSet()
//--
//-- Mix user set limits with script set limits and defaults.
//--
bool wzapi::applyLimitSet(WZAPI_NO_PARAMS)
{
	return ::applyLimitSet();
}

//-- ## setMissionTime(time)
//--
//-- Set mission countdown in seconds.
//--
wzapi::no_return_value wzapi::setMissionTime(WZAPI_PARAMS(int _value))
{
	int value = _value * GAME_TICKS_PER_SEC;
	mission.startTime = gameTime;
	mission.time = value;
	setMissionCountDown();
	if (mission.time >= 0)
	{
		mission.startTime = gameTime;
		addMissionTimerInterface();
	}
	else
	{
		intRemoveMissionTimer();
		mission.cheatTime = 0;
	}
	return {};
}

//-- ## getMissionTime()
//--
//-- Get time remaining on mission countdown in seconds. (3.2+ only)
//--
int wzapi::getMissionTime(WZAPI_NO_PARAMS)
{
	return ((mission.time - (gameTime - mission.startTime)) / GAME_TICKS_PER_SEC);
}

