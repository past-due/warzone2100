/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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
 * @file quickjs_backend.cpp
 *
 * New scripting system -- script functions
 */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtCore/QFileInfo>

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


#include <unordered_set>
#include "lib/framework/file.h"
#include <unordered_map>

#include "quickjs.h"
#include "3rdparty/gsl_finally.h"

struct JSContextValue {
	JSContext *ctx;
	JSValue value;
};
void to_json(nlohmann::json& j, const JSContextValue& value); // forward-declare

class quickjs_scripting_instance;
static std::map<JSContext*, quickjs_scripting_instance *> engineToInstanceMap;

class quickjs_scripting_instance : public wzapi::scripting_instance
{
public:
	quickjs_scripting_instance(int player, const std::string& scriptName)
	: scripting_instance(player, scriptName)
	{
		rt = JS_NewRuntime();
		ctx = JS_NewContext(rt);
		global_obj = JS_GetGlobalObject(ctx);

		engineToInstanceMap.insert(std::pair<JSContext*, quickjs_scripting_instance*>(ctx, this));
	}
	virtual ~quickjs_scripting_instance()
	{
		engineToInstanceMap.erase(ctx);

		JS_FreeValue(ctx, global_obj);
		JS_FreeContext(ctx);
		ctx = nullptr;
		JS_FreeRuntime(rt);
		rt = nullptr;
	}
	bool loadScript(const WzString& path, int player, int difficulty);
	bool readyInstanceForExecution() override;

private:
	bool registerFunctions(const std::string& scriptName);

public:
	virtual bool isReceivingAllEvents() const override
	{
		return false; // TODO: IMPLEMENT
		//return (engine->globalObject().property("isReceivingAllEvents")).toBool();
	}

public:
	// save / restore state
	virtual bool saveScriptGlobals(nlohmann::json &result) override;
	virtual bool loadScriptGlobals(const nlohmann::json &result) override;

	virtual nlohmann::json saveTimerFunction(uniqueTimerID timerID, std::string timerName, timerAdditionalData* additionalParam) override;

	// recreates timer functions (and additional userdata) based on the information saved by the saveTimerFunction() method
	virtual std::tuple<TimerFunc, timerAdditionalData *> restoreTimerFunction(const nlohmann::json& savedTimerFuncData) override;

public:
	// get state for debugging
	nlohmann::json debugGetAllScriptGlobals() override;

	bool debugEvaluateCommand(const std::string &text) override;

public:

	void updateGameTime(uint32_t gameTime) override;
	void updateGroupSizes(int group, int size) override;

	void setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags = wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave) override;

public:

	void doNotSaveGlobal(const std::string &global);

private:
	JSRuntime *rt;
    JSContext *ctx;
	JSValue global_obj;

	JSValue compiledScriptObj = JS_UNINITIALIZED;
	std::string m_path;
	/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
	std::unordered_set<std::string> internalNamespace;
	/// Separate event namespaces for libraries
public: // temporary
	std::vector<std::string> eventNamespaces;
	JSValue Get_Global_Obj() const { return global_obj; }

public:
	// MARK: General events

	//__ ## eventGameInit()
	//__
	//__ An event that is run once as the game is initialized. Not all game state may have been
	//__ properly initialized by this time, so use this only to initialize script state.
	//__
	virtual bool handle_eventGameInit() override;

	//__ ## eventStartLevel()
	//__
	//__ An event that is run once the game has started and all game data has been loaded.
	//__
	virtual bool handle_eventStartLevel() override;

	//__ ## eventMissionTimeout()
	//__
	//__ An event that is run when the mission timer has run out.
	//__
	virtual bool handle_eventMissionTimeout() override;

	//__ ## eventVideoDone()
	//__
	//__ An event that is run when a video show stopped playing.
	//__
	virtual bool handle_eventVideoDone() override;

	//__ ## eventGameLoaded()
	//__
	//__ An event that is run when game is loaded from a saved game. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameLoaded() override;

	//__ ## eventGameSaving()
	//__
	//__ An event that is run before game is saved. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameSaving() override;

	//__ ## eventGameSaved()
	//__
	//__ An event that is run after game is saved. There is usually no need to use this event.
	//__
	virtual bool handle_eventGameSaved() override;

public:
	// MARK: Transporter events

	//__
	//__ ## eventTransporterLaunch(transport)
	//__
	//__ An event that is run when the mission transporter has been ordered to fly off.
	//__
	virtual bool handle_eventLaunchTransporter() override; // DEPRECATED!
	virtual bool handle_eventTransporterLaunch(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterArrived(transport)
	//__
	//__ An event that is run when the mission transporter has arrived at the map edge with reinforcements.
	//__
	virtual bool handle_eventReinforcementsArrived() override; // DEPRECATED!
	virtual bool handle_eventTransporterArrived(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterExit(transport)
	//__
	//__ An event that is run when the mission transporter has left the map.
	//__
	virtual bool handle_eventTransporterExit(const BASE_OBJECT *psObj) override;

	//__ ## eventTransporterDone(transport)
	//__
	//__ An event that is run when the mission transporter has no more reinforcements to deliver.
	//__
	virtual bool handle_eventTransporterDone(const BASE_OBJECT *psTransport) override;

	//__ ## eventTransporterLanded(transport)
	//__
	//__ An event that is run when the mission transporter has landed with reinforcements.
	//__
	virtual bool handle_eventTransporterLanded(const BASE_OBJECT *psTransport) override;

public:
	// MARK: UI-related events (intended for the tutorial)

	//__ ## eventDeliveryPointMoving()
	//__
	//__ An event that is run when the current player starts to move a delivery point.
	//__
	virtual bool handle_eventDeliveryPointMoving(const BASE_OBJECT *psStruct) override;

	//__ ## eventDeliveryPointMoved()
	//__
	//__ An event that is run after the current player has moved a delivery point.
	//__
	virtual bool handle_eventDeliveryPointMoved(const BASE_OBJECT *psStruct) override;

	//__ ## eventDesignBody()
	//__
	//__An event that is run when current user picks a body in the design menu.
	//__
	virtual bool handle_eventDesignBody() override;

	//__ ## eventDesignPropulsion()
	//__
	//__An event that is run when current user picks a propulsion in the design menu.
	//__
	virtual bool handle_eventDesignPropulsion() override;

	//__ ## eventDesignWeapon()
	//__
	//__An event that is run when current user picks a weapon in the design menu.
	//__
	virtual bool handle_eventDesignWeapon() override;

	//__ ## eventDesignCommand()
	//__
	//__An event that is run when current user picks a command turret in the design menu.
	//__
	virtual bool handle_eventDesignCommand() override;

	//__ ## eventDesignSystem()
	//__
	//__An event that is run when current user picks a system other than command turret in the design menu.
	//__
	virtual bool handle_eventDesignSystem() override;

	//__ ## eventDesignQuit()
	//__
	//__An event that is run when current user leaves the design menu.
	//__
	virtual bool handle_eventDesignQuit() override;

	//__ ## eventMenuBuildSelected()
	//__
	//__An event that is run when current user picks something new in the build menu.
	//__
	virtual bool handle_eventMenuBuildSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuResearchSelected()
	//__
	//__An event that is run when current user picks something new in the research menu.
	//__
	virtual bool handle_eventMenuResearchSelected(/*BASE_OBJECT *psObj*/) override;

	//__ ## eventMenuBuild()
	//__
	//__An event that is run when current user opens the build menu.
	//__
	virtual bool handle_eventMenuBuild() override;

	//__ ## eventMenuResearch()
	//__
	//__An event that is run when current user opens the research menu.
	//__
	virtual bool handle_eventMenuResearch() override;


	virtual bool handle_eventMenuDesign() override;

	//__ ## eventMenuManufacture()
	//__An event that is run when current user opens the manufacture menu.
	//__
	virtual bool handle_eventMenuManufacture() override;

	//__ ## eventSelectionChanged(objects)
	//__
	//__ An event that is triggered whenever the host player selects one or more game objects.
	//__ The ```objects``` parameter contains an array of the currently selected game objects.
	//__ Keep in mind that the player may drag and drop select many units at once, select one
	//__ unit specifically, or even add more selections to a current selection one at a time.
	//__ This event will trigger once for each user action, not once for each selected or
	//__ deselected object. If all selected game objects are deselected, ```objects``` will
	//__ be empty.
	//__
	virtual bool handle_eventSelectionChanged(const std::vector<const BASE_OBJECT *>& objects) override;

public:
	// MARK: Game state-change events

	//__ ## eventObjectRecycled()
	//__
	//__ An event that is run when an object (ex. droid, structure) is recycled.
	//__
	virtual bool handle_eventObjectRecycled(const BASE_OBJECT *psObj) override;

	//__ ## eventPlayerLeft(player index)
	//__
	//__ An event that is run after a player has left the game.
	//__
	virtual bool handle_eventPlayerLeft(int id) override;

	//__ ## eventCheatMode(entered)
	//__
	//__ Game entered or left cheat/debug mode.
	//__ The entered parameter is true if cheat mode entered, false otherwise.
	//__
	virtual bool handle_eventCheatMode(bool entered) override;

	//__ ## eventDroidIdle(droid)
	//__
	//__ A droid should be given new orders.
	//__
	virtual bool handle_eventDroidIdle(const DROID *psDroid) override;

	//__ ## eventDroidBuilt(droid[, structure])
	//__
	//__ An event that is run every time a droid is built. The structure parameter is set
	//__ if the droid was produced in a factory. It is not triggered for droid theft or
	//__ gift (check ```eventObjectTransfer``` for that).
	//__
	virtual bool handle_eventDroidBuilt(const DROID *psDroid, const STRUCTURE *psFactory) override;

	//__ ## eventStructureBuilt(structure[, droid])
	//__
	//__ An event that is run every time a structure is produced. The droid parameter is set
	//__ if the structure was built by a droid. It is not triggered for building theft
	//__ (check ```eventObjectTransfer``` for that).
	//__
	virtual bool handle_eventStructBuilt(const STRUCTURE *psStruct, const DROID *psDroid) override;

	//__ ## eventStructureDemolish(structure[, droid])
	//__
	//__ An event that is run every time a structure begins to be demolished. This does
	//__ not trigger again if the structure is partially demolished.
	//__
	virtual bool handle_eventStructDemolish(const STRUCTURE *psStruct, const DROID *psDroid) override;

	//__ ## eventStructureReady(structure)
	//__
	//__ An event that is run every time a structure is ready to perform some
	//__ special ability. It will only fire once, so if the time is not right,
	//__ register your own timer to keep checking.
	//__
	virtual bool handle_eventStructureReady(const STRUCTURE *psStruct) override;

	//__ ## eventAttacked(victim, attacker)
	//__
	//__ An event that is run when an object belonging to the script's controlling player is
	//__ attacked. The attacker parameter may be either a structure or a droid.
	//__
	virtual bool handle_eventAttacked(const BASE_OBJECT *psVictim, const BASE_OBJECT *psAttacker) override;

	//__ ## eventResearched(research, structure, player)
	//__
	//__ An event that is run whenever a new research is available. The structure
	//__ parameter is set if the research comes from a research lab owned by the
	//__ current player. If an ally does the research, the structure parameter will
	//__ be set to null. The player parameter gives the player it is called for.
	//__
	virtual bool handle_eventResearched(const wzapi::researchResult& research, const STRUCTURE *psStruct, int player) override;

	//__ ## eventDestroyed(object)
	//__
	//__ An event that is run whenever an object is destroyed. Careful passing
	//__ the parameter object around, since it is about to vanish!
	//__
	virtual bool handle_eventDestroyed(const BASE_OBJECT *psVictim) override;

	//__ ## eventPickup(feature, droid)
	//__
	//__ An event that is run whenever a feature is picked up. It is called for
	//__ all players / scripts.
	//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
	//__
	virtual bool handle_eventPickup(const FEATURE *psFeat, const DROID *psDroid) override;

	//__ ## eventObjectSeen(viewer, seen)
	//__
	//__ An event that is run sometimes when an object, which was marked by an object label,
	//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
	//__ An event that is run sometimes when an objectm  goes from not seen to seen.
	//__ First parameter is **game object** doing the seeing, the next the game
	//__ object being seen.
	virtual bool handle_eventObjectSeen(const BASE_OBJECT *psViewer, const BASE_OBJECT *psSeen) override;

	//__
	//__ ## eventGroupSeen(viewer, group)
	//__
	//__ An event that is run sometimes when a member of a group, which was marked by a group label,
	//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
	//__ First parameter is **game object** doing the seeing, the next the id of the group
	//__ being seen.
	//__
	virtual bool handle_eventGroupSeen(const BASE_OBJECT *psViewer, int groupId) override;

	//__ ## eventObjectTransfer(object, from)
	//__
	//__ An event that is run whenever an object is transferred between players,
	//__ for example due to a Nexus Link weapon. The event is called after the
	//__ object has been transferred, so the target player is in object.player.
	//__ The event is called for both players.
	//__
	virtual bool handle_eventObjectTransfer(const BASE_OBJECT *psObj, int from) override;

	//__ ## eventChat(from, to, message)
	//__
	//__ An event that is run whenever a chat message is received. The ```from``` parameter is the
	//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
	//__ player.
	//__
	virtual bool handle_eventChat(int from, int to, const char *message) override;

	//__ ## eventBeacon(x, y, from, to[, message])
	//__
	//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__ Message may be undefined.
	//__
	virtual bool handle_eventBeacon(int x, int y, int from, int to, const char *message) override;

	//__ ## eventBeaconRemoved(from, to)
	//__
	//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
	//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
	//__
	virtual bool handle_eventBeaconRemoved(int from, int to) override;

	//__ ## eventGroupLoss(object, group id, new size)
	//__
	//__ An event that is run whenever a group becomes empty. Input parameter
	//__ is the about to be killed object, the group's id, and the new group size.
	//__
//		// Since groups are entities local to one context, we do not iterate over them here.
	virtual bool handle_eventGroupLoss(const BASE_OBJECT *psObj, int group, int size) override;

	//__ ## eventArea<label>(droid)
	//__
	//__ An event that is run whenever a droid enters an area label. The area is then
	//__ deactived. Call resetArea() to reactivate it. The name of the event is
	//__ eventArea + the name of the label.
	//__
	virtual bool handle_eventArea(const std::string& label, const DROID *psDroid) override;

	//__ ## eventDesignCreated(template)
	//__
	//__ An event that is run whenever a new droid template is created. It is only
	//__ run on the client of the player designing the template.
	//__
	virtual bool handle_eventDesignCreated(const DROID_TEMPLATE *psTemplate) override;

	//__ ## eventAllianceOffer(from, to)
	//__
	//__ An event that is called whenever an alliance offer is requested.
	//__
	virtual bool handle_eventAllianceOffer(uint8_t from, uint8_t to) override;

	//__ ## eventAllianceAccepted(from, to)
	//__
	//__ An event that is called whenever an alliance is accepted.
	//__
	virtual bool handle_eventAllianceAccepted(uint8_t from, uint8_t to) override;

	//__ ## eventAllianceBroken(from, to)
	//__
	//__ An event that is called whenever an alliance is broken.
	//__
	virtual bool handle_eventAllianceBroken(uint8_t from, uint8_t to) override;

public:
	// MARK: Special input events

	//__ ## eventSyncRequest(req_id, x, y, obj_id, obj_id2)
	//__
	//__ An event that is called from a script and synchronized with all other scripts and hosts
	//__ to prevent desync from happening. Sync requests must be carefully validated to prevent
	//__ cheating!
	//__
	virtual bool handle_eventSyncRequest(int from, int req_id, int x, int y, const BASE_OBJECT *psObj, const BASE_OBJECT *psObj2) override;

	//__ ## eventKeyPressed(meta, key)
	//__
	//__ An event that is called whenever user presses a key in the game, not counting chat
	//__ or other pop-up user interfaces. The key values are currently undocumented.
	virtual bool handle_eventKeyPressed(int meta, int key) override;
};

//#define ALL_PLAYERS -1
//#define ALLIES -2
//#define ENEMIES -3

//Vector2i positions[MAX_PLAYERS];
//std::vector<Vector2i> derricks;
//
//void scriptSetStartPos(int position, int x, int y)
//{
//	positions[position].x = x;
//	positions[position].y = y;
//	debug(LOG_SCRIPT, "Setting start position %d to (%d, %d)", position, x, y);
//}
//
//void scriptSetDerrickPos(int x, int y)
//{
//	Vector2i pos(x, y);
//	derricks.push_back(pos);
//}
//
//bool scriptInit()
//{
//	int i;
//
//	for (i = 0; i < MAX_PLAYERS; i++)
//	{
//		scriptSetStartPos(i, 0, 0);
//	}
//	derricks.clear();
//	derricks.reserve(8 * MAX_PLAYERS);
//	return true;
//}
//
//Vector2i getPlayerStartPosition(int player)
//{
//	return positions[player];
//}

// private qtscript bureaucracy

/// Assert for scripts that give useful backtraces and other info.
#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			JS_ThrowReferenceError(context, "%s failed in %s at line %d", #expr, __FUNCTION__, __LINE__); \
			return JS_NULL; } } while (0)

#define SCRIPT_ASSERT_PLAYER(_context, _player) \
	SCRIPT_ASSERT(_context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

#define WzStringToQScriptValue(_engine, _wzstring) \
	QScriptValue(_engine, QString::fromUtf8(_wzstring.toUtf8().c_str()))

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

JSValue mapJsonToQuickJSValue(JSContext *ctx, const nlohmann::json &instance, uint8_t prop_flags); // forward-declare

static JSValue mapJsonObjectToQuickJSValue(JSContext *ctx, const nlohmann::json &obj, uint8_t prop_flags)
{
	JSValue value = JS_NewObject(ctx);
    JSAtom prop_name;
    int ret;
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		JSValue prop_val = mapJsonToQuickJSValue(ctx, it.value(), prop_flags);
//		JS_SetPropertyStr(ctx, value, it.key().c_str(), childVal);
		//value.setProperty(QString::fromUtf8(it.key().c_str()), mapJsonToQScriptValue(engine, it.value(), flags), flags);
		prop_name = JS_NewAtom(ctx, it.key().c_str());
		ret = JS_DefinePropertyValue(ctx, value, prop_name, prop_val, prop_flags);
		JS_FreeAtom(ctx, prop_name);
	}
	return value;
}

static JSValue mapJsonArrayToQuickJSValue(JSContext *ctx, const nlohmann::json &array, uint8_t prop_flags)
{
	JSValue value = JS_NewArray(ctx);
	for (int i = 0; i < array.size(); i++)
	{
		JSValue prop_val = mapJsonToQuickJSValue(ctx, array.at(i), prop_flags);
		JS_DefinePropertyValueUint32(ctx, value, i, prop_val, prop_flags);
//		value.setProperty(i, mapJsonToQScriptValue(engine, array.at(i), flags), flags);
	}
	return value;
}

JSValue mapJsonToQuickJSValue(JSContext *ctx, const nlohmann::json &instance, uint8_t prop_flags)
{
	switch (instance.type())
	{
		// IMPORTANT: To match the prior behavior of loading a QVariant from the JSON value and using engine->toScriptValue(QVariant)
		//			  to convert to a QScriptValue, "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//
		case json::value_t::null : return JS_UNDEFINED;
		case json::value_t::boolean : return JS_NewBool(ctx, instance.get<bool>()); //engine->toScriptValue(instance.get<bool>());
		case json::value_t::number_integer: return JS_NewInt32(ctx, instance.get<int32_t>()); //engine->toScriptValue(instance.get<int>());
		case json::value_t::number_unsigned: return JS_NewUint32(ctx, instance.get<uint32_t>()); //engine->toScriptValue(instance.get<unsigned>());
		case json::value_t::number_float: return JS_NewFloat64(ctx, instance.get<double>()); //engine->toScriptValue(instance.get<double>());
		case json::value_t::string	: return JS_NewString(ctx, instance.get<WzString>().toUtf8().c_str()); //engine->toScriptValue(QString::fromUtf8(instance.get<WzString>().toUtf8().c_str()));
		case json::value_t::array : return mapJsonArrayToQuickJSValue(ctx, instance, prop_flags);
		case json::value_t::object : return mapJsonObjectToQuickJSValue(ctx, instance, prop_flags);
		case json::value_t::binary :
			debug(LOG_ERROR, "Unexpected binary value type");
			return JS_UNDEFINED;
		case json::value_t::discarded : return JS_UNDEFINED;
	}
	return JS_UNDEFINED; // should never be reached
}

// Forward-declare
JSValue convDroid(const DROID *psDroid, JSContext *ctx);
JSValue convStructure(const STRUCTURE *psStruct, JSContext *ctx);
JSValue convObj(const BASE_OBJECT *psObj, JSContext *ctx);
JSValue convFeature(const FEATURE *psFeature, JSContext *ctx);
JSValue convMax(const BASE_OBJECT *psObj, JSContext *ctx);
JSValue convTemplate(const DROID_TEMPLATE *psTemplate, JSContext *ctx);
JSValue convResearch(const RESEARCH *psResearch, JSContext *ctx, int player);

static int QuickJS_DefinePropertyValue(JSContext *ctx, JSValueConst this_obj, const char* prop, JSValue val, int flags)
{
	JSAtom prop_name = JS_NewAtom(ctx, prop);
	int ret = JS_DefinePropertyValue(ctx, this_obj, prop_name, val, flags);
	JS_FreeAtom(ctx, prop_name);
	return ret;
}

//;; ## Research
//;;
//;; Describes a research item. The following properties are defined:
//;;
//;; * ```power``` Number of power points needed for starting the research.
//;; * ```points``` Number of research points needed to complete the research.
//;; * ```started``` A boolean saying whether or not this research has been started by current player or any of its allies.
//;; * ```done``` A boolean saying whether or not this research has been completed.
//;; * ```name``` A string containing the full name of the research.
//;; * ```id``` A string containing the index name of the research.
//;; * ```type``` The type will always be ```RESEARCH_DATA```.
//;;
JSValue convResearch(const RESEARCH *psResearch, JSContext *ctx, int player)
{
	if (psResearch == nullptr)
	{
		return JS_NULL;
	}
	JSValue value = JS_NewObject(ctx);
	QuickJS_DefinePropertyValue(ctx, value, "power", JS_NewInt32(ctx, (int)psResearch->researchPower), 0);
	QuickJS_DefinePropertyValue(ctx, value, "points", JS_NewInt32(ctx, (int)psResearch->researchPoints), 0);
	bool started = false;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (aiCheckAlliances(player, i) || player == i)
		{
			int bits = asPlayerResList[i][psResearch->index].ResearchStatus;
			started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING) || (bits & RESBITS_PENDING_ONLY);
		}
	}
	QuickJS_DefinePropertyValue(ctx, value, "started", JS_NewBool(ctx, started), 0); // including whether an ally has started it
	QuickJS_DefinePropertyValue(ctx, value, "done", JS_NewBool(ctx, IsResearchCompleted(&asPlayerResList[player][psResearch->index])), 0);
	QuickJS_DefinePropertyValue(ctx, value, "fullname", JS_NewString(ctx, psResearch->name.toUtf8().c_str()), 0); // temporary
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, psResearch->id.toUtf8().c_str()), 0); // will be changed to contain fullname
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewString(ctx, psResearch->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "type", JS_NewInt32(ctx, SCRIPT_RESEARCH), 0);
	QuickJS_DefinePropertyValue(ctx, value, "results", mapJsonToQuickJSValue(ctx, psResearch->results, 0), 0);
	return value;
}

//;; ## Structure
//;;
//;; Describes a structure (building). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```status``` The completeness status of the structure. It will be one of ```BEING_BUILT``` and ```BUILT```.
//;; * ```type``` The type will always be ```STRUCTURE```.
//;; * ```cost``` What it would cost to build this structure. (3.2+ only)
//;; * ```stattype``` The stattype defines the type of structure. It will be one of ```HQ```, ```FACTORY```, ```POWER_GEN```,
//;; ```RESOURCE_EXTRACTOR```, ```LASSAT```, ```DEFENSE```, ```WALL```, ```RESEARCH_LAB```, ```REPAIR_FACILITY```,
//;; ```CYBORG_FACTORY```, ```VTOL_FACTORY```, ```REARM_PAD```, ```SAT_UPLINK```, ```GATE``` and ```COMMAND_CONTROL```.
//;; * ```modules``` If the stattype is set to one of the factories, ```POWER_GEN``` or ```RESEARCH_LAB```, then this property is set to the
//;; number of module upgrades it has.
//;; * ```canHitAir``` True if the structure has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the structure has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the structure has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the structure has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the structure has radar detector ability. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```hasIndirect``` One or more of the structure's weapons are indirect. (3.2+ only)
//;;
JSValue convStructure(const STRUCTURE *psStruct, JSContext *ctx)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	for (int i = 0; i < psStruct->numWeaps; i++)
	{
		if (psStruct->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psStruct->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psStruct->player), range);
		}
	}
	JSValue value = convObj(psStruct, ctx);
	QuickJS_DefinePropertyValue(ctx, value, "isCB", JS_NewBool(ctx, structCBSensor(psStruct)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "isSensor", JS_NewBool(ctx, structStandardSensor(psStruct)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "canHitAir", JS_NewBool(ctx, aa), 0);
	QuickJS_DefinePropertyValue(ctx, value, "canHitGround", JS_NewBool(ctx, ga), 0);
	QuickJS_DefinePropertyValue(ctx, value, "hasIndirect", JS_NewBool(ctx, indirect), 0);
	QuickJS_DefinePropertyValue(ctx, value, "isRadarDetector", JS_NewBool(ctx, objRadarDetector(psStruct)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "range", JS_NewInt32(ctx, range), 0);
	QuickJS_DefinePropertyValue(ctx, value, "status", JS_NewInt32(ctx, (int)psStruct->status), 0);
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewInt32(ctx, 100 * psStruct->body / MAX(1, structureBody(psStruct))), 0);
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewInt32(ctx, psStruct->pStructureType->powerToBuild), 0);
	int stattype = 0;
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		stattype = (int)REF_WALL;
		break;
	case REF_GENERIC:
	case REF_DEFENSE:
		stattype = (int)REF_DEFENSE;
		break;
	default:
		stattype = (int)psStruct->pStructureType->type;
		break;
	}
	QuickJS_DefinePropertyValue(ctx, value, "stattype", JS_NewInt32(ctx, stattype), 0);
	if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	    || psStruct->pStructureType->type == REF_VTOL_FACTORY
	    || psStruct->pStructureType->type == REF_RESEARCH
	    || psStruct->pStructureType->type == REF_POWER_GEN)
	{
		QuickJS_DefinePropertyValue(ctx, value, "modules", JS_NewUint32(ctx, psStruct->capacity), 0);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "modules", JS_NULL, 0);
	}
	JSValue weaponlist = JS_NewArray(ctx); //engine->newArray(psStruct->numWeaps);
	for (int j = 0; j < psStruct->numWeaps; j++)
	{
		JSValue weapon = JS_NewObject(ctx);
		const WEAPON_STATS *psStats = asWeaponStats + psStruct->asWeaps[j].nStat;
		QuickJS_DefinePropertyValue(ctx, weapon, "fullname", JS_NewString(ctx, psStats->name.toUtf8().c_str()), 0);
		QuickJS_DefinePropertyValue(ctx, weapon, "name", JS_NewString(ctx, psStats->id.toUtf8().c_str()), 0); // will be changed to contain full name
		QuickJS_DefinePropertyValue(ctx, weapon, "id", JS_NewString(ctx, psStats->id.toUtf8().c_str()), 0);
		QuickJS_DefinePropertyValue(ctx, weapon, "lastFired", JS_NewUint32(ctx, psStruct->asWeaps[j].lastFired), 0);
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, weapon, 0);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, 0);
	return value;
}

//;; ## Feature
//;;
//;; Describes a feature (a **game object** not owned by any player). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; * ```type``` It will always be ```FEATURE```.
//;; * ```stattype``` The type of feature. Defined types are ```OIL_RESOURCE```, ```OIL_DRUM``` and ```ARTIFACT```.
//;; * ```damageable``` Can this feature be damaged?
//;;
JSValue convFeature(const FEATURE *psFeature, JSContext *ctx)
{
	JSValue value = convObj(psFeature, ctx);
	const FEATURE_STATS *psStats = psFeature->psStats;
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewUint32(ctx, 100 * psStats->body / MAX(1, psFeature->body)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "damageable", JS_NewBool(ctx, psStats->damageable), 0);
	QuickJS_DefinePropertyValue(ctx, value, "stattype", JS_NewInt32(ctx, psStats->subType), 0);
	return value;
}

//;; ## Droid
//;;
//;; Describes a droid. It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```type``` It will always be ```DROID```.
//;; * ```order``` The current order of the droid. This is its plan. The following orders are defined:
//;;   * ```DORDER_ATTACK``` Order a droid to attack something.
//;;   * ```DORDER_MOVE``` Order a droid to move somewhere.
//;;   * ```DORDER_SCOUT``` Order a droid to move somewhere and stop to attack anything on the way.
//;;   * ```DORDER_BUILD``` Order a droid to build something.
//;;   * ```DORDER_HELPBUILD``` Order a droid to help build something.
//;;   * ```DORDER_LINEBUILD``` Order a droid to build something repeatedly in a line.
//;;   * ```DORDER_REPAIR``` Order a droid to repair something.
//;;   * ```DORDER_PATROL``` Order a droid to patrol.
//;;   * ```DORDER_DEMOLISH``` Order a droid to demolish something.
//;;   * ```DORDER_EMBARK``` Order a droid to embark on a transport.
//;;   * ```DORDER_DISEMBARK``` Order a transport to disembark its units at the given position.
//;;   * ```DORDER_FIRESUPPORT``` Order a droid to fire at whatever the target sensor is targeting. (3.2+ only)
//;;   * ```DORDER_COMMANDERSUPPORT``` Assign the droid to a commander. (3.2+ only)
//;;   * ```DORDER_STOP``` Order a droid to stop whatever it is doing. (3.2+ only)
//;;   * ```DORDER_RTR``` Order a droid to return for repairs. (3.2+ only)
//;;   * ```DORDER_RTB``` Order a droid to return to base. (3.2+ only)
//;;   * ```DORDER_HOLD``` Order a droid to hold its position. (3.2+ only)
//;;   * ```DORDER_REARM``` Order a VTOL droid to rearm. If given a target, will go to specified rearm pad. If not, will go to nearest rearm pad. (3.2+ only)
//;;   * ```DORDER_OBSERVE``` Order a droid to keep a target in sensor view. (3.2+ only)
//;;   * ```DORDER_RECOVER``` Order a droid to pick up something. (3.2+ only)
//;;   * ```DORDER_RECYCLE``` Order a droid to factory for recycling. (3.2+ only)
//;; * ```action``` The current action of the droid. This is how it intends to carry out its plan. The
//;; C++ code may change the action frequently as it tries to carry out its order. You never want to set
//;; the action directly, but it may be interesting to look at what it currently is.
//;; * ```droidType``` The droid's type. The following types are defined:
//;;   * ```DROID_CONSTRUCT``` Trucks and cyborg constructors.
//;;   * ```DROID_WEAPON``` Droids with weapon turrets, except cyborgs.
//;;   * ```DROID_PERSON``` Non-cyborg two-legged units, like scavengers.
//;;   * ```DROID_REPAIR``` Units with repair turret, including repair cyborgs.
//;;   * ```DROID_SENSOR``` Units with sensor turret.
//;;   * ```DROID_ECM``` Unit with ECM jammer turret.
//;;   * ```DROID_CYBORG``` Cyborgs with weapons.
//;;   * ```DROID_TRANSPORTER``` Cyborg transporter.
//;;   * ```DROID_SUPERTRANSPORTER``` Droid transporter.
//;;   * ```DROID_COMMAND``` Commanders.
//;; * ```group``` The group this droid is member of. This is a numerical ID. If not a member of any group, will be set to \emph{null}.
//;; * ```armed``` The percentage of weapon capability that is fully armed. Will be \emph{null} for droids other than VTOLs.
//;; * ```experience``` Amount of experience this droid has, based on damage it has dealt to enemies.
//;; * ```cost``` What it would cost to build the droid. (3.2+ only)
//;; * ```isVTOL``` True if the droid is VTOL. (3.2+ only)
//;; * ```canHitAir``` True if the droid has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the droid has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the droid has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the droid has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the droid has radar detector ability. (3.2+ only)
//;; * ```hasIndirect``` One or more of the droid's weapons are indirect. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```body``` The body component of the droid. (3.2+ only)
//;; * ```propulsion``` The propulsion component of the droid. (3.2+ only)
//;; * ```weapons``` The weapon components of the droid, as an array. Contains 'name', 'id', 'armed' percentage and 'lastFired' properties. (3.2+ only)
//;; * ```cargoCapacity``` Defined for transporters only: Total cargo capacity (number of items that will fit may depend on their size). (3.2+ only)
//;; * ```cargoSpace``` Defined for transporters only: Cargo capacity left. (3.2+ only)
//;; * ```cargoCount``` Defined for transporters only: Number of individual \emph{items} in the cargo hold. (3.2+ only)
//;; * ```cargoSize``` The amount of cargo space the droid will take inside a transport. (3.2+ only)
//;;
JSValue convDroid(const DROID *psDroid, JSContext *ctx)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	const BODY_STATS *psBodyStats = &asBodyStats[psDroid->asBits[COMP_BODY]];

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psDroid->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psDroid->player), range);
		}
	}
	DROID_TYPE type = psDroid->droidType;
	JSValue value = convObj(psDroid, ctx);
	QuickJS_DefinePropertyValue(ctx, value, "action", JS_NewInt32(ctx, (int)psDroid->action), 0);
	if (range >= 0)
	{
		QuickJS_DefinePropertyValue(ctx, value, "range", JS_NewInt32(ctx, range), 0);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "range", JS_NULL, 0);
	}
	QuickJS_DefinePropertyValue(ctx, value, "order", JS_NewInt32(ctx, (int)psDroid->order.type), 0);
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewUint32(ctx, calcDroidPower(psDroid)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "hasIndirect", JS_NewBool(ctx, indirect), 0);
	switch (psDroid->droidType) // hide some engine craziness
	{
	case DROID_CYBORG_CONSTRUCT:
		type = DROID_CONSTRUCT; break;
	case DROID_CYBORG_SUPER:
		type = DROID_CYBORG; break;
	case DROID_DEFAULT:
		type = DROID_WEAPON; break;
	case DROID_CYBORG_REPAIR:
		type = DROID_REPAIR; break;
	default:
		break;
	}
	QuickJS_DefinePropertyValue(ctx, value, "bodySize", JS_NewInt32(ctx, psBodyStats->size), 0);
	if (isTransporter(psDroid))
	{
		QuickJS_DefinePropertyValue(ctx, value, "cargoCapacity", JS_NewInt32(ctx, TRANSPORTER_CAPACITY), 0);
		QuickJS_DefinePropertyValue(ctx, value, "cargoLeft", JS_NewInt32(ctx, calcRemainingCapacity(psDroid)), 0);
		QuickJS_DefinePropertyValue(ctx, value, "cargoCount", JS_NewUint32(ctx, psDroid->psGroup != nullptr? psDroid->psGroup->getNumMembers() : 0), 0);
	}
	QuickJS_DefinePropertyValue(ctx, value, "isRadarDetector", JS_NewBool(ctx, objRadarDetector(psDroid)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "isCB", JS_NewBool(ctx, cbSensorDroid(psDroid)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "isSensor", JS_NewBool(ctx, standardSensorDroid(psDroid)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "canHitAir", JS_NewBool(ctx, aa), 0);
	QuickJS_DefinePropertyValue(ctx, value, "canHitGround", JS_NewBool(ctx, ga), 0);
	QuickJS_DefinePropertyValue(ctx, value, "isVTOL", JS_NewBool(ctx, isVtolDroid(psDroid)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "droidType", JS_NewInt32(ctx, (int)type), 0);
	QuickJS_DefinePropertyValue(ctx, value, "experience", JS_NewFloat64(ctx, (double)psDroid->experience / 65536.0), 0);
	QuickJS_DefinePropertyValue(ctx, value, "health", JS_NewFloat64(ctx, 100.0 / (double)psDroid->originalBody * (double)psDroid->body), 0);

	QuickJS_DefinePropertyValue(ctx, value, "body", JS_NewString(ctx, asBodyStats[psDroid->asBits[COMP_BODY]].id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "propulsion", JS_NewString(ctx, asPropulsionStats[psDroid->asBits[COMP_PROPULSION]].id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "armed", JS_NewFloat64(ctx, 0.0), 0); // deprecated!

	JSValue weaponlist = JS_NewArray(ctx); //engine->newArray(psDroid->numWeaps);
	for (int j = 0; j < psDroid->numWeaps; j++)
	{
		int armed = droidReloadBar(psDroid, &psDroid->asWeaps[j], j);
		JSValue weapon = JS_NewObject(ctx);
		const WEAPON_STATS *psStats = asWeaponStats + psDroid->asWeaps[j].nStat;
		QuickJS_DefinePropertyValue(ctx, weapon, "fullname", JS_NewString(ctx, psStats->name.toUtf8().c_str()), 0);
		QuickJS_DefinePropertyValue(ctx, weapon, "name", JS_NewString(ctx, psStats->id.toUtf8().c_str()), 0); // will be changed to contain full name
		QuickJS_DefinePropertyValue(ctx, weapon, "id", JS_NewString(ctx, psStats->id.toUtf8().c_str()), 0);
		QuickJS_DefinePropertyValue(ctx, weapon, "lastFired", JS_NewUint32(ctx, psDroid->asWeaps[j].lastFired), 0);
		QuickJS_DefinePropertyValue(ctx, weapon, "armed", JS_NewInt32(ctx, armed), 0);
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, weapon, 0);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, 0);
	QuickJS_DefinePropertyValue(ctx, value, "cargoSize", JS_NewInt32(ctx, transporterSpaceRequired(psDroid)), 0);
	return value;
}

//;; ## Base Object
//;;
//;; Describes a basic object. It will always be a droid, structure or feature, but sometimes the
//;; difference does not matter, and you can treat any of them simply as a basic object. These
//;; fields are also inherited by the droid, structure and feature objects.
//;; The following properties are defined:
//;;
//;; * ```type``` It will be one of ```DROID```, ```STRUCTURE``` or ```FEATURE```.
//;; * ```id``` The unique ID of this object.
//;; * ```x``` X position of the object in tiles.
//;; * ```y``` Y position of the object in tiles.
//;; * ```z``` Z (height) position of the object in tiles.
//;; * ```player``` The player owning this object.
//;; * ```selected``` A boolean saying whether 'selectedPlayer' has selected this object.
//;; * ```name``` A user-friendly name for this object.
//;; * ```health``` Percentage that this object is damaged (where 100 means not damaged at all).
//;; * ```armour``` Amount of armour points that protect against kinetic weapons.
//;; * ```thermal``` Amount of thermal protection that protect against heat based weapons.
//;; * ```born``` The game time at which this object was produced or came into the world. (3.2+ only)
//;;
JSValue convObj(const BASE_OBJECT *psObj, JSContext *ctx)
{
	JSValue value = JS_NewObject(ctx);
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewUint32(ctx, psObj->id), 0);
	QuickJS_DefinePropertyValue(ctx, value, "x", JS_NewInt32(ctx, map_coord(psObj->pos.x)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "y", JS_NewInt32(ctx, map_coord(psObj->pos.y)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "z", JS_NewInt32(ctx, map_coord(psObj->pos.z)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "player", JS_NewUint32(ctx, psObj->player), 0);
	QuickJS_DefinePropertyValue(ctx, value, "armour", JS_NewInt32(ctx, objArmour(psObj, WC_KINETIC)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "thermal", JS_NewInt32(ctx, objArmour(psObj, WC_HEAT)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "type", JS_NewInt32(ctx, psObj->type), 0);
	QuickJS_DefinePropertyValue(ctx, value, "selected", JS_NewUint32(ctx, psObj->selected), 0);
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, objInfo(psObj)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "born", JS_NewUint32(ctx, psObj->born), 0);
	scripting_engine::GROUPMAP *psMap = scripting_engine::instance().getGroupMap(engineToInstanceMap.at(ctx));
	if (psMap != nullptr && psMap->map().count(psObj) > 0) // FIXME:
	{
		int group = psMap->map().at(psObj); // FIXME:
		QuickJS_DefinePropertyValue(ctx, value, "group", JS_NewInt32(ctx, group), 0);
	}
	else
	{
		QuickJS_DefinePropertyValue(ctx, value, "group", JS_NULL, 0);
	}
	return value;
}

//;; ## Template
//;;
//;; Describes a template type. Templates are droid designs that a player has created.
//;; The following properties are defined:
//;;
//;; * ```id``` The ID of this object.
//;; * ```name``` Name of the template.
//;; * ```cost``` The power cost of the template if put into production.
//;; * ```droidType``` The type of droid that would be created.
//;; * ```body``` The name of the body type.
//;; * ```propulsion``` The name of the propulsion type.
//;; * ```brain``` The name of the brain type.
//;; * ```repair``` The name of the repair type.
//;; * ```ecm``` The name of the ECM (electronic counter-measure) type.
//;; * ```construct``` The name of the construction type.
//;; * ```weapons``` An array of weapon names attached to this template.
JSValue convTemplate(const DROID_TEMPLATE *psTempl, JSContext *ctx)
{
	JSValue value = JS_NewObject(ctx);
	ASSERT_OR_RETURN(value, psTempl, "No object for conversion");
	QuickJS_DefinePropertyValue(ctx, value, "fullname", JS_NewString(ctx, psTempl->name.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "name", JS_NewString(ctx, psTempl->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "id", JS_NewString(ctx, psTempl->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "points", JS_NewUint32(ctx, calcTemplateBuild(psTempl)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "power", JS_NewUint32(ctx, calcTemplatePower(psTempl)), 0); // deprecated, use cost below
	QuickJS_DefinePropertyValue(ctx, value, "cost", JS_NewUint32(ctx, calcTemplatePower(psTempl)), 0);
	QuickJS_DefinePropertyValue(ctx, value, "droidType", JS_NewInt32(ctx, psTempl->droidType), 0);
	QuickJS_DefinePropertyValue(ctx, value, "body", JS_NewString(ctx, (asBodyStats + psTempl->asParts[COMP_BODY])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "propulsion", JS_NewString(ctx, (asPropulsionStats + psTempl->asParts[COMP_PROPULSION])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "brain", JS_NewString(ctx, (asBrainStats + psTempl->asParts[COMP_BRAIN])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "repair", JS_NewString(ctx, (asRepairStats + psTempl->asParts[COMP_REPAIRUNIT])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "ecm", JS_NewString(ctx, (asECMStats + psTempl->asParts[COMP_ECM])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "sensor", JS_NewString(ctx, (asSensorStats + psTempl->asParts[COMP_SENSOR])->id.toUtf8().c_str()), 0);
	QuickJS_DefinePropertyValue(ctx, value, "construct", JS_NewString(ctx, (asConstructStats + psTempl->asParts[COMP_CONSTRUCT])->id.toUtf8().c_str()), 0);
	JSValue weaponlist = JS_NewArray(ctx); //engine->newArray(psTempl->numWeaps);
	for (int j = 0; j < psTempl->numWeaps; j++)
	{
		JS_DefinePropertyValueUint32(ctx, weaponlist, j, JS_NewString(ctx, (asWeaponStats + psTempl->asWeaps[j])->id.toUtf8().c_str()), 0);
	}
	QuickJS_DefinePropertyValue(ctx, value, "weapons", weaponlist, 0);
	return value;
}

JSValue convMax(const BASE_OBJECT *psObj, JSContext *ctx)
{
	if (!psObj)
	{
		return JS_NULL;
//		return QScriptValue::NullValue;
	}
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((const DROID *)psObj, ctx);
	case OBJ_STRUCTURE: return convStructure((const STRUCTURE *)psObj, ctx);
	case OBJ_FEATURE: return convFeature((const FEATURE *)psObj, ctx);
	default: ASSERT(false, "No such supported object type"); return convObj(psObj, ctx);
	}
}

BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player)
{
	switch (type)
	{
	case OBJ_DROID: return IdToDroid(id, player);
	case OBJ_FEATURE: return IdToFeature(id, player);
	case OBJ_STRUCTURE: return IdToStruct(id, player);
	default: return nullptr;
	}
}

// Call a function by name
static JSValue callFunction(JSContext *ctx, const std::string &function, std::vector<JSValue> &args, bool event = true)
{
	const auto instance = engineToInstanceMap.at(ctx);
	JSValue global_obj = JS_GetGlobalObject(ctx);
	if (event)
	{
		// recurse into variants, if any
		for (const std::string &s : instance->eventNamespaces)
		{
			std::string funcName = s + function;
			JSValue value = JS_GetPropertyStr(ctx, global_obj, funcName.c_str()); // engine->globalObject().property(s + function);
			if (JS_IsFunction(ctx, value))
			{
				callFunction(ctx, funcName, args, event);
			}
		}
	}
	code_part level = event ? LOG_SCRIPT : LOG_ERROR;
	JSValue value = JS_GetPropertyStr(ctx, global_obj, function.c_str()); //engine->globalObject().property(function);
	if (!JS_IsFunction(ctx, value))
	{
		// not necessarily an error, may just be a trigger that is not defined (ie not needed)
		// or it could be a typo in the function name or ...
		debug(level, "called function (%s) not defined", function.c_str());
		return JS_FALSE; // ?? Shouldn't this be "undefined?"
	}

	JSValue result;
	scripting_engine::instance().executeWithPerformanceMonitoring(instance, function, [ctx, &result, &value, &args](){
		result = JS_Call(ctx, value, JS_UNDEFINED, (int)args.size(), args.data()); //value.call(QScriptValue(), args);
	});

	if (JS_IsException(result))
	{
		JSValue err = JS_GetException(ctx);
		bool isError = JS_IsError(ctx, err);
		std::string result_str;
		if (isError)
		{
			JSValue name = JS_GetPropertyStr(ctx, err, "name");
			const char* errorname_str = JS_ToCString(ctx, name);
			result_str = (errorname_str) ? errorname_str : "<unknown error name>";
			result_str += ":: ";
			JS_FreeCString(ctx, errorname_str);
			JS_FreeValue(ctx, name);
			JSValue stack = JS_GetPropertyStr(ctx, err, "stack");
			if (!JS_IsUndefined(stack)) {
                const char* stack_str = JS_ToCString(ctx, stack);
                if (stack_str) {
					result_str += stack_str;
//                    const char *p;
//                    int len;
//
////                    if (outfile)
////                        fprintf(outfile, "%s", stack_str);
//
//                    len = strlen(filename);
//                    p = strstr(stack_str, filename);
//                    if (p != NULL && p[len] == ':') {
//                        error_line = atoi(p + len + 1);
//                        has_error_line = TRUE;
//                    }
                    JS_FreeCString(ctx, stack_str);
                }
            }
            JS_FreeValue(ctx, stack);
		}
		int line = 0; // TODO: //engine->uncaughtExceptionLineNumber();
//		QStringList bt = engine->uncaughtExceptionBacktrace();
//		for (int i = 0; i < bt.size(); i++)
//		{
//			debug(LOG_ERROR, "%d : %s", i, bt.at(i).toUtf8().constData());
//		}
		ASSERT(false, "Uncaught exception calling function \"%s\" at line %d: %s",
		       function.c_str(), line, result_str.c_str());
//		engine->clearExceptions();
		return JS_UNINITIALIZED;
	}
	return result;
}

// ----------------------------------------------------------------------------------------

	class quickjs_execution_context : public wzapi::execution_context
	{
	private:
		JSContext *ctx = nullptr;
	public:
		quickjs_execution_context(JSContext *ctx)
		: ctx(ctx)
		{ }
		~quickjs_execution_context() { }
	public:
		virtual wzapi::scripting_instance* currentInstance() const override
		{
			return engineToInstanceMap.at(ctx);
		}

		virtual int player() const override
		{
			JSValue global_obj = JS_GetGlobalObject(ctx);
			JSValue value = JS_GetPropertyStr(ctx, global_obj, "me");
			int32_t player = -1;
			if (JS_ToInt32(ctx, &player, value))
			{
				// failed
				ASSERT(false, "Failed"); // TODO:
			}
			JS_FreeValue(ctx, global_obj);
			return player;
		}

		virtual void throwError(const char *expr, int line, const char *function) const override
		{
			JS_ThrowReferenceError(ctx, "%s failed in %s at line %d", expr, function, line);
//			context->throwError(QScriptContext::ReferenceError, QString(expr) +  " failed in " + QString(function) + " at line " + QString::number(line));
		}

		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const override
		{
			JSContext *pCtx = ctx;
			return [pCtx, func](const int player) {
				std::vector<JSValue> args;
				args.push_back(JS_NewInt32(pCtx, player));
				callFunction(pCtx, func.toUtf8(), args);
				std::for_each(args.begin(), args.end(), [pCtx](JSValue& val) { JS_FreeValue(pCtx, val); });
			};
		}

		virtual void hack_setMe(int player) const override
		{
			JSValue global_obj = JS_GetGlobalObject(ctx);
			QuickJS_DefinePropertyValue(ctx, global_obj, "me", JS_NewInt32(ctx, player), 0);
//			engine->globalObject().setProperty("me", player);
			JS_FreeValue(ctx, global_obj);
		}

		virtual void set_isReceivingAllEvents(bool value) const override
		{
			JSValue global_obj = JS_GetGlobalObject(ctx);
			QuickJS_DefinePropertyValue(ctx, global_obj, "isReceivingAllEvents", JS_NewBool(ctx, value), 0);
//			engine->globalObject().setProperty("isReceivingAllEvents", value, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			JS_FreeValue(ctx, global_obj);
		}

		virtual bool get_isReceivingAllEvents() const override
		{
			JSValue global_obj = JS_GetGlobalObject(ctx);
			JSValue value = JS_GetPropertyStr(ctx, global_obj, "isReceivingAllEvents");
			bool result = JS_ToBool(ctx, value);
			JS_FreeValue(ctx, value);
			JS_FreeValue(ctx, global_obj);
			return result;
//			return (engine->globalObject().property("isReceivingAllEvents")).toBool();
		}

		virtual void doNotSaveGlobal(const std::string &name) const override
		{
			engineToInstanceMap.at(ctx)->doNotSaveGlobal(name);
		}
	};

	/// Assert for scripts that give useful backtraces and other info.
	#define UNBOX_SCRIPT_ASSERT(context, expr, ...) \
		do { bool _wzeval = (expr); \
			if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
				JS_ThrowReferenceError(ctx, "%s failed when converting argument %zu for %s", #expr, idx, function); \
				break; } } while (0)

	namespace
	{
		template<typename T>
		struct unbox {
			//T operator()(size_t& idx, QScriptContext *context) const;
		};

		template<>
		struct unbox<int>
		{
			int operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				int32_t value = -1;
				if (JS_ToInt32(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		template<>
		struct unbox<unsigned int>
		{
			unsigned int operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				uint32_t value = -1;
				if (JS_ToUint32(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		template<>
		struct unbox<bool>
		{
			bool operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				return JS_ToBool(ctx, argv[idx++]);
			}
		};



		template<>
		struct unbox<float>
		{
			float operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				double value = -1;
				if (JS_ToFloat64(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return static_cast<float>(value);
			}
		};

		template<>
		struct unbox<double>
		{
			double operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				double value = -1;
				if (JS_ToFloat64(ctx, &value, argv[idx++]))
				{
					// failed
					ASSERT(false, "Failed"); // TODO:
				}
				return value;
			}
		};

		static inline int32_t JSValueToInt32(JSContext *ctx, JSValue &value)
		{
			int intVal = -1;
			if (JS_ToInt32(ctx, &intVal, value))
			{
				// failed
				ASSERT(false, "Failed"); // TODO:
			}
			JS_FreeValue(ctx, value);
			return intVal;
		}

		static int32_t QuickJS_GetInt32(JSContext *ctx, JSValueConst this_obj, const char *prop)
		{
			JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
			int32_t result = JSValueToInt32(ctx, val);
			JS_FreeValue(ctx, val);
			return result;
		}

		static inline std::string JSValueToStdString(JSContext *ctx, JSValue &value)
		{
			const char* pStr = JS_ToCString(ctx, value);
			std::string result;
			if (pStr) result = pStr;
			JS_FreeCString(ctx, pStr);
			return result;
		}

		static std::string QuickJS_GetStdString(JSContext *ctx, JSValueConst this_obj, const char *prop)
		{
			JSValue val = JS_GetPropertyStr(ctx, this_obj, prop);
			std::string result = JSValueToStdString(ctx, val);
			JS_FreeValue(ctx, val);
			return result;
		}

		bool QuickJS_GetArrayLength(JSContext* ctx, JSValueConst arr, uint64_t& len)
		{
			if (!JS_IsArray(ctx, arr))
				return false;

			JSValue len_val = JS_GetPropertyStr(ctx, arr, "length");
			if (JS_IsException(len_val))
				return false;

			if (JS_ToIndex(ctx, &len, len_val)) {
				JS_FreeValue(ctx, len_val);
				return false;
			}

			JS_FreeValue(ctx, len_val);
			return 0;
		}

		template<>
		struct unbox<DROID*>
		{
			DROID* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue droidVal = argv[idx++];
				int id = QuickJS_GetInt32(ctx, droidVal, "id");
				int player = QuickJS_GetInt32(ctx, droidVal, "player");
				DROID *psDroid = IdToDroid(id, player);
				UNBOX_SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
				return psDroid;
			}
		};

		template<>
		struct unbox<const DROID*>
		{
			const DROID* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<DROID*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<STRUCTURE*>
		{
			STRUCTURE* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue structVal = argv[idx++];
				int id = QuickJS_GetInt32(ctx, structVal, "id");
				int player = QuickJS_GetInt32(ctx, structVal, "player");
				STRUCTURE *psStruct = IdToStruct(id, player);
				UNBOX_SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
				return psStruct;
			}
		};

		template<>
		struct unbox<const STRUCTURE*>
		{
			const STRUCTURE* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<STRUCTURE*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<BASE_OBJECT*>
		{
			BASE_OBJECT* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				JSValue objVal = argv[idx++];
				int oid = QuickJS_GetInt32(ctx, objVal, "id");
				int oplayer = QuickJS_GetInt32(ctx, objVal, "player");
				OBJECT_TYPE otype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, objVal, "type");
				BASE_OBJECT* psObj = IdToObject(otype, oid, oplayer);
				UNBOX_SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
				return psObj;
			}
		};

		template<>
		struct unbox<const BASE_OBJECT*>
		{
			const BASE_OBJECT* operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				return unbox<BASE_OBJECT*>()(idx, ctx, argc, argv, function);
			}
		};

		template<>
		struct unbox<std::string>
		{
			std::string operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				return JSValueToStdString(ctx, argv[idx++]);
			}
		};

		template<>
		struct unbox<wzapi::STRUCTURE_TYPE_or_statsName_string>
		{
			wzapi::STRUCTURE_TYPE_or_statsName_string operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				wzapi::STRUCTURE_TYPE_or_statsName_string result;
				if (argc <= idx)
					return result;
				JSValue val = argv[idx++];
				if (JS_IsNumber(val))
				{
					result.type = (STRUCTURE_TYPE)JSValueToInt32(ctx, val);
				}
				else
				{
					result.statsName = JSValueToStdString(ctx, val);
				}
				return result;
			}
		};

		template<typename OptionalType>
		struct unbox<optional<OptionalType>>
		{
			optional<OptionalType> operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				return optional<OptionalType>(unbox<OptionalType>()(idx, ctx, argc, argv, function));
			}
		};

		template<>
		struct unbox<wzapi::reservedParam>
		{
			wzapi::reservedParam operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				// just ignore parameter value, and increment idx
				idx++;
				return {};
			}
		};

		template<>
		struct unbox<wzapi::game_object_identifier>
		{
			wzapi::game_object_identifier operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc < idx)
					return {};
				JSValue objVal = argv[idx++];
				wzapi::game_object_identifier result;
				result.id = QuickJS_GetInt32(ctx, objVal, "id");
				result.player = QuickJS_GetInt32(ctx, objVal, "player");
				result.type = QuickJS_GetInt32(ctx, objVal, "type");
				return result;
			}
		};

		template<>
		struct unbox<wzapi::string_or_string_list>
		{
			wzapi::string_or_string_list operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::string_or_string_list strings;

				JSValue list_or_string = argv[idx++];
				if (JS_IsArray(ctx, list_or_string))
				{
					uint64_t length = 0;
					if (QuickJS_GetArrayLength(ctx, list_or_string, length))
					{
						for (uint64_t k = 0; k < length; k++) // TODO: uint64_t isn't correct here, as we call GetUint32...
						{
							JSValue resVal = JS_GetPropertyUint32(ctx, list_or_string, k);
							strings.strings.push_back(JSValueToStdString(ctx, resVal));
							JS_FreeValue(ctx, resVal);
						}
					}
				}
				else
				{
					strings.strings.push_back(JSValueToStdString(ctx, list_or_string));
				}
				return strings;
			}
		};

		template<>
		struct unbox<wzapi::va_list_treat_as_strings>
		{
			wzapi::va_list_treat_as_strings operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::va_list_treat_as_strings strings;
				for (; idx < argc; idx++)
				{
					const char* pStr = JS_ToCString(ctx, argv[idx]);
					if (!pStr) // (context->state() == QScriptContext::ExceptionState)
					{
						break;
					}
					strings.strings.push_back(std::string(pStr));
					JS_FreeCString(ctx, pStr);
				}
				return strings;
			}
		};

		template<typename ContainedType>
		struct unbox<wzapi::va_list<ContainedType>>
		{
			wzapi::va_list<ContainedType> operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};
				wzapi::va_list<ContainedType> result;
				for (; idx < argc; idx++)
				{
					result.va_list.push_back(unbox<ContainedType>()(idx, ctx, argc, argv, function));
				}
				return result;
			}
		};

		template<>
		struct unbox<scripting_engine::area_by_values_or_area_label_lookup>
		{
			scripting_engine::area_by_values_or_area_label_lookup operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				if (JS_IsString(argv[idx]))
				{
					return scripting_engine::area_by_values_or_area_label_lookup(JSValueToStdString(ctx, argv[idx++]));
				}
				else if ((argc - idx) >= 4)
				{
					int x1, y1, x2, y2;
					x1 = JSValueToInt32(ctx, argv[idx]); //context->argument(idx).toInt32();
					y1 = JSValueToInt32(ctx, argv[idx + 1]); //context->argument(idx + 1).toInt32();
					x2 = JSValueToInt32(ctx, argv[idx + 2]); //context->argument(idx + 2).toInt32();
					y2 = JSValueToInt32(ctx, argv[idx + 3]); //context->argument(idx + 3).toInt32();
					idx += 4;
					return scripting_engine::area_by_values_or_area_label_lookup(x1, y1, x2, y2);
				}
				else
				{
					// could log an error here
				}
				return {};
			}
		};

		template<>
		struct unbox<generic_script_object>
		{
			generic_script_object operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				JSValue qval = argv[idx++];
				int type = QuickJS_GetInt32(ctx, qval, "type"); //qval.property("type").toInt32();

				JSValue triggered = JS_GetPropertyStr(ctx, qval, "triggered");
				if (JS_IsNumber(triggered))
				{
//					UNBOX_SCRIPT_ASSERT(context, type != SCRIPT_POSITION, "Cannot assign a trigger to a position");
					ASSERT(false, "Not currently handling triggered property - does anything use this?");
				}

				if (type == SCRIPT_RADIUS)
				{
					return generic_script_object::fromRadius(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y"),
						QuickJS_GetInt32(ctx, qval, "radius")
					);
				}
				else if (type == SCRIPT_AREA)
				{
					return generic_script_object::fromArea(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y"),
						QuickJS_GetInt32(ctx, qval, "x2"),
						QuickJS_GetInt32(ctx, qval, "y2")
					);
				}
				else if (type == SCRIPT_POSITION)
				{
					return generic_script_object::fromPosition(
						QuickJS_GetInt32(ctx, qval, "x"),
						QuickJS_GetInt32(ctx, qval, "y")
					);
				}
				else if (type == SCRIPT_GROUP)
				{
					return generic_script_object::fromGroup(
						QuickJS_GetInt32(ctx, qval, "id")
					);
				}
				else if (type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE)
				{
					int id = QuickJS_GetInt32(ctx, qval, "id");
					int player = QuickJS_GetInt32(ctx, qval, "player");
					BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)type, id, player);
					UNBOX_SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", id, player); // TODO: fail out
					return generic_script_object::fromObject(psObj);
				}
				return {};
			}
		};

		template<>
		struct unbox<wzapi::object_request>
		{
			wzapi::object_request operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if (argc <= idx)
					return {};

				if ((argc - idx) >= 3) // get by ID case (3 parameters)
				{
					OBJECT_TYPE type = (OBJECT_TYPE)JSValueToInt32(ctx, argv[idx++]);
					int player = JSValueToInt32(ctx, argv[idx++]);
					int id = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::object_request(type, player, id);
				}
				else if ((argc - idx) >= 2) // get at position case (2 parameters)
				{
					int x = JSValueToInt32(ctx, argv[idx++]);
					int y = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::object_request(x, y);
				}
				else
				{
					// get by label case (1 parameter)
					std::string label = JSValueToStdString(ctx, argv[idx++]);
					return wzapi::object_request(label);
				}
			}
		};

		template<>
		struct unbox<wzapi::label_or_position_values>
		{
			wzapi::label_or_position_values operator()(size_t& idx, JSContext *ctx, int argc, JSValueConst *argv, const char *function)
			{
				if ((argc - idx) >= 4) // square area
				{
					int x1 = JSValueToInt32(ctx, argv[idx++]);
					int y1 = JSValueToInt32(ctx, argv[idx++]);
					int x2 = JSValueToInt32(ctx, argv[idx++]);
					int y2 = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::label_or_position_values(x1, y1, x2, y2);
				}
				else if ((argc - idx) >= 2) // single tile
				{
					int x = JSValueToInt32(ctx, argv[idx++]);
					int y = JSValueToInt32(ctx, argv[idx++]);
					return wzapi::label_or_position_values(x, y);
				}
				else if ((argc - idx) >= 1) // label
				{
					std::string label = JSValueToStdString(ctx, argv[idx++]);
					return wzapi::label_or_position_values(label);
				}
				return wzapi::label_or_position_values();
			}
		};

		template<typename T>
		JSValue box(T a, JSContext *);
//		{
//			return QScriptValue(a);
//		}

		JSValue box(const char* str, JSContext* ctx)
		{
			return JS_NewString(ctx, str);
		}

		JSValue box(std::string str, JSContext* ctx)
		{
			return JS_NewStringLen(ctx, str.c_str(), str.length());
		}

		JSValue box(wzapi::no_return_value, JSContext* ctx)
		{
			return JS_UNDEFINED;
		}

		JSValue box(const BASE_OBJECT * psObj, JSContext* ctx)
		{
			if (!psObj)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convMax(psObj, ctx);
		}

		JSValue box(const STRUCTURE * psStruct, JSContext* ctx)
		{
			if (!psStruct)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convStructure(psStruct, ctx);
		}

		JSValue box(const DROID * psDroid, JSContext* ctx)
		{
			if (!psDroid)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convDroid(psDroid, ctx);
		}

		JSValue box(const FEATURE * psFeat, JSContext* ctx)
		{
			if (!psFeat)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convFeature(psFeat, ctx);
		}

		JSValue box(const DROID_TEMPLATE * psTemplate, JSContext* ctx)
		{
			if (!psTemplate)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convTemplate(psTemplate, ctx);
		}

//		// deliberately not defined
//		// use `wzapi::researchResult` instead of `const RESEARCH *`
//		template<>
//		QScriptValue box(const RESEARCH * psResearch, QScriptEngine* engine);

		JSValue box(const scr_radius& r, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_RADIUS), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, r.x), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, r.y), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "radius", JS_NewInt32(ctx, r.radius), 0);
			return ret;
		}

		JSValue box(const scr_area& r, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_AREA), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, r.x1), 0); // TODO: Rename scr_area x1 to x
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, r.y1), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "x2", JS_NewInt32(ctx, r.x2), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "y2", JS_NewInt32(ctx, r.y2), 0);
			return ret;
		}

		JSValue box(const scr_position& p, JSContext* ctx)
		{
			JSValue ret = JS_NewObject(ctx);
			QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, SCRIPT_POSITION), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "x", JS_NewInt32(ctx, p.x), 0);
			QuickJS_DefinePropertyValue(ctx, ret, "y", JS_NewInt32(ctx, p.y), 0);
			return ret;
		}

		JSValue box(const generic_script_object& p, JSContext* ctx)
		{
			int type = p.getType();
			switch (type)
			{
			case SCRIPT_RADIUS:
				return box(p.getRadius(), ctx);
				break;
			case SCRIPT_AREA:
				return box(p.getArea(), ctx);
				break;
			case SCRIPT_POSITION:
				return box(p.getPosition(), ctx);
				break;
			case SCRIPT_GROUP:
			{
				JSValue ret = JS_NewObject(ctx);
				QuickJS_DefinePropertyValue(ctx, ret, "type", JS_NewInt32(ctx, type), 0);
				QuickJS_DefinePropertyValue(ctx, ret, "id", JS_NewInt32(ctx, p.getGroupId()), 0);
				return ret;
			}
				break;
			case OBJ_DROID:
			case OBJ_FEATURE:
			case OBJ_STRUCTURE:
			{
				BASE_OBJECT* psObj = p.getObject();
				return convMax(psObj, ctx);
			}
				break;
			default:
				debug(LOG_SCRIPT, "Unsupported object label type: %d", type);
				break;
			}

			return JS_UNINITIALIZED;
			//return QScriptValue();
		}

		template<typename VectorType>
		JSValue box(const std::vector<VectorType>& value, JSContext* ctx)
		{
			JSValue result = JS_NewArray(ctx); //engine->newArray(value.size());
			for (int i = 0; i < value.size(); i++)
			{
				VectorType item = value.at(i);
				JS_DefinePropertyValueUint32(ctx, result, i, box(item, ctx), 0); // TODO: Check return value?
			}
			return result;
		}

		JSValue box(const wzapi::researchResult& result, JSContext* ctx)
		{
			if (result.psResearch == nullptr)
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
			return convResearch(result.psResearch, ctx, result.player);
		}

		JSValue box(const wzapi::researchResults& results, JSContext* ctx)
		{
			JSValue result = JS_NewArray(ctx); //engine->newArray(results.resList.size());
			for (int i = 0; i < results.resList.size(); i++)
			{
				const RESEARCH *psResearch = results.resList.at(i);
				JS_DefinePropertyValueUint32(ctx, result, i, convResearch(psResearch, ctx, results.player), 0); // TODO: Check return value?
			}
			return result;
		}

		template<typename OptionalType>
		JSValue box(const optional<OptionalType>& result, JSContext* ctx)
		{
			if (result.has_value())
			{
				return box(result.value(), ctx);
			}
			else
			{
				return JS_UNDEFINED;
				//return QScriptValue(); // "undefined" variable
				//return QScriptValue::NullValue;
			}
		}

		template<typename PtrType>
		JSValue box(std::unique_ptr<PtrType> result, JSContext* ctx)
		{
			if (result)
			{
				return box(result.get(), ctx);
			}
			else
			{
				return JS_NULL;
//				return QScriptValue::NullValue;
			}
		}

		#include <cstddef>
		#include <tuple>
		#include <type_traits>
		#include <utility>

		template<size_t N>
		struct Apply {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T && t, A &&... a)
				-> decltype(Apply<N-1>::apply(
					::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				))
			{
				return Apply<N-1>::apply(::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				);
			}
		};

		template<>
		struct Apply<0> {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T &&, A &&... a)
				-> decltype(::std::forward<F>(f)(::std::forward<A>(a)...))
			{
				return ::std::forward<F>(f)(::std::forward<A>(a)...);
			}
		};

		template<typename F, typename T>
		inline auto apply(F && f, T && t)
			-> decltype(Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t)))
		{
			return Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t));
		}

		MSVC_PRAGMA(warning( push )) // see matching "pop" below
		MSVC_PRAGMA(warning( disable : 4189 )) // disable "warning C4189: 'idx': local variable is initialized but not referenced"
		
		template<typename R, typename...Args>
		JSValue wrap__(R(*f)(const wzapi::execution_context&, Args...), WZ_DECL_UNUSED const char *wrappedFunctionName, JSContext *context, int argc, JSValueConst *argv)
		{
			size_t idx WZ_DECL_UNUSED = 0; // unused when Args... is empty
			quickjs_execution_context execution_context(context);
			return box(apply(f, std::tuple<const wzapi::execution_context&, Args...>{static_cast<const wzapi::execution_context&>(execution_context), unbox<Args>{}(idx, context, argc, argv, wrappedFunctionName)...}), context);
		}

		template<typename R, typename...Args>
		JSValue wrap__(R(*f)(), WZ_DECL_UNUSED const char *wrappedFunctionName, JSContext *context, int argc, JSValueConst *argv)
		{
			return box(f(), context);
		}

		MSVC_PRAGMA(warning( pop ))

		#define wrap_(wzapi_function, context, argc, argv) \
		wrap__(wzapi_function, #wzapi_function, context, argc, argv)

		template <typename T>
		void append_value_list(std::vector<JSValue> &list, T t, JSContext *context) { list.push_back(box(std::forward<T>(t), context)); }

		template <typename... Args>
		bool wrap_event_handler__(const std::string &functionName, JSContext *context, Args&&... args)
		{
			std::vector<JSValue> args_list;
			using expander = int[];
//			WZ_DECL_UNUSED int dummy[] = { 0, ((void) append_value_list(args_list, std::forward<Args>(args), engine),0)... };
			// Left-most void to avoid `expression result unused [-Wunused-value]`
			(void)expander{ 0, ((void) append_value_list(args_list, std::forward<Args>(args), context),0)... };
			/*QScriptValue result =*/ callFunction(context, functionName, args_list);
			std::for_each(args_list.begin(), args_list.end(), [context](JSValue& val) { JS_FreeValue(context, val); });
			return true; //nlohmann::json(result.toVariant());
		}

		/* PP_NARG returns the number of arguments that have been passed to it.
		 * (Expand these macros as necessary. Does not support 0 arguments.)
		 */
		#define WRAP_EXPAND(x) x
        #define VARGS_COUNT_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
        #define VARGS_COUNT(...) WRAP_EXPAND(VARGS_COUNT_N(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0))

		static_assert(VARGS_COUNT(A) == 1, "PP_NARG() failed for 1 argument");
		static_assert(VARGS_COUNT(A,B) == 2, "PP_NARG() failed for 2 arguments");
		static_assert(VARGS_COUNT(A,B,C,D,E,F,G,H,I,J) == 10, "PP_NARG() failed for 12 arguments");

		#define MAKE_PARAMS_1(t) t arg1
		#define MAKE_PARAMS_2(t1, t2) t1 arg1, t2 arg2
		#define MAKE_PARAMS_3(t1, t2, t3) t1 arg1, t2 arg2, t3 arg3
		#define MAKE_PARAMS_4(t1, t2, t3, t4) t1 arg1, t2 arg2, t3 arg3, t4 arg4
		#define MAKE_PARAMS_5(t1, t2, t3, t4, t5) t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5
		#define MAKE_PARAMS_6(t1, t2, t3, t4, t5, t6) t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5, t6 arg6

		#define MAKE_ARGS_1(t) arg1
		#define MAKE_ARGS_2(t1, t2) arg1, arg2
		#define MAKE_ARGS_3(t1, t2, t3) arg1, arg2, arg3
		#define MAKE_ARGS_4(t1, t2, t3, t4) arg1, arg2, arg3, arg4
		#define MAKE_ARGS_5(t1, t2, t3, t4, t5) arg1, arg2, arg3, arg4, arg5
		#define MAKE_ARGS_6(t1, t2, t3, t4, t5, t6) arg1, arg2, arg3, arg4, arg5, arg6

		#define MAKE_PARAMS_COUNT(COUNT, ...) WRAP_EXPAND(MAKE_PARAMS_##COUNT(__VA_ARGS__))
		#define MAKE_PARAMS_WITH_COUNT(COUNT, ...) MAKE_PARAMS_COUNT(COUNT, __VA_ARGS__)
		#define MAKE_PARAMS(...) MAKE_PARAMS_WITH_COUNT(VARGS_COUNT(__VA_ARGS__), __VA_ARGS__)

		#define MAKE_ARGS_COUNT(COUNT, ...) WRAP_EXPAND(MAKE_ARGS_##COUNT(__VA_ARGS__))
		#define MAKE_ARGS_WITH_COUNT(COUNT, ...) MAKE_ARGS_COUNT(COUNT, __VA_ARGS__)
		#define MAKE_ARGS(...) MAKE_ARGS_WITH_COUNT(VARGS_COUNT(__VA_ARGS__), __VA_ARGS__)

		#define STRINGIFY_EXPAND(tok) #tok
		#define STRINGIFY(tok) STRINGIFY_EXPAND(tok)

		#define IMPL_EVENT_HANDLER(fun, ...) \
			bool qtscript_scripting_instance::handle_##fun(MAKE_PARAMS(__VA_ARGS__)) { \
				return wrap_event_handler__(STRINGIFY(fun), engine, MAKE_ARGS(__VA_ARGS__)); \
			}

		#define IMPL_EVENT_HANDLER_NO_PARAMS(fun) \
		bool qtscript_scripting_instance::handle_##fun() { \
			return wrap_event_handler__(STRINGIFY(fun), engine); \
		}

	}

// Wraps a QScriptEngine instance

//-- ## profile(function[, arguments])
//-- Calls a function with given arguments, measures time it took to evaluate the function,
//-- and adds this time to performance monitor statistics. Transparently returns the
//-- function's return value. The function to run is the first parameter, and it
//-- _must be quoted_. (3.2+ only)
//--
static JSValue js_profile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 1, "Must have at least one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Profiled functions must be quoted");
	std::string funcName = JSValueToStdString(ctx, argv[0]);
	std::vector<JSValue> args;
	for (int i = 1; i < argc; ++i)
	{
		args.push_back(argv[i]);
	}
	return callFunction(ctx, funcName, args);
}

static std::string QuickJS_DumpObject(JSContext *ctx, JSValue obj)
{
	std::string result;
    const char *str = JS_ToCString(ctx, obj);
    if (str)
	{
		result = str;
        JS_FreeCString(ctx, str);
    }
	else
	{
        result = "[exception]";
    }
	return result;
}

static std::string QuickJS_DumpError(JSContext *ctx)
{
	std::string result;
    JSValue exception_val = JS_GetException(ctx);
	bool isError = JS_IsError(ctx, exception_val);
	result = QuickJS_DumpObject(ctx, exception_val);
	if (isError)
	{
		JSValue stack = JS_GetPropertyStr(ctx, exception_val, "stack");
		if (!JS_IsUndefined(stack))
		{
			result += "\n" + QuickJS_DumpObject(ctx, stack);
		}
		JS_FreeValue(ctx, stack);
	}
    JS_FreeValue(ctx, exception_val);
	return result;
}

//-- ## include(file)
//-- Includes another source code file at this point. You should generally only specify the filename,
//-- not try to specify its path, here.
//--
static JSValue js_include(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must specify a file to include");
	JSValue global_obj = JS_GetGlobalObject(ctx);
	std::string basePath = QuickJS_GetStdString(ctx, global_obj, "scriptPath"); //engine->globalObject().property("scriptPath").toString();
	std::string basenameStr = JSValueToStdString(ctx, argv[0]);
	QFileInfo basename(basenameStr.c_str());
	std::string path = basePath + "/" + basename.fileName().toStdString();
	// allow users to use subdirectories too
	if (PHYSFS_exists(basename.filePath().toUtf8().constData()))
	{
		path = basename.filePath().toStdString(); // use this path instead (from read-only dir)
	}
	else if (PHYSFS_exists(QString("scripts/" + basename.filePath()).toUtf8().constData()))
	{
		path = "scripts/" + basename.filePath().toStdString(); // use this path instead (in user write dir)
	}
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\" (path=%s, name=%s)",
		      path.c_str(), basePath.c_str(), basename.filePath().toUtf8().constData());
		JS_ThrowReferenceError(ctx, "Failed to read include file \"%s\" (path=%s, name=%s)", path.c_str(), basePath.c_str(), basename.filePath().toUtf8().constData());
		return JS_FALSE;
	}
//	QString source = QString::fromUtf8(bytes, size);
//	free(bytes);
//	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
//	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
//	{
//		debug(LOG_ERROR, "Syntax error in include %s line %d: %s",
//		      path.toUtf8().constData(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
//		return JS_FALSE;
//	}
//	context->setActivationObject(engine->globalObject());
//	context->setThisObject(engine->globalObject());
	JSValue result = JS_Eval(ctx, bytes, size, path.c_str(), JS_EVAL_TYPE_GLOBAL); // QScriptValue result = engine->evaluate(source, path);
	free(bytes);
	if (JS_IsException(result)) // (engine->hasUncaughtException())
	{
		std::string errorAsString = QuickJS_DumpError(ctx);
//		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception in include file %s: %s",
		      path.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return JS_FALSE;
    }
    JS_FreeValue(ctx, result);
	debug(LOG_SCRIPT, "Included new script file %s", path.c_str());
	return JS_TRUE;
}

class quickjs_timer_additionaldata : public timerAdditionalData
{
public:
	std::string stringArg;
	quickjs_timer_additionaldata(const std::string& _stringArg)
	: stringArg(_stringArg)
	{ }
};

//-- ## setTimer(function, milliseconds[, object])
//--
//-- Set a function to run repeated at some given time interval. The function to run
//-- is the first parameter, and it _must be quoted_, otherwise the function will
//-- be inlined. The second parameter is the interval, in milliseconds. A third, optional
//-- parameter can be a **game object** to pass to the timer function. If the **game object**
//-- dies, the timer stops running. The minimum number of milliseconds is 100, but such
//-- fast timers are strongly discouraged as they may deteriorate the game performance.
//--
//-- ```javascript
//--   function conDroids()
//--   {
//--      ... do stuff ...
//--   }
//--   // call conDroids every 4 seconds
//--   setTimer("conDroids", 4000);
//-- ```
//--
static JSValue js_setTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 2, "Must have at least two parameters");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Timer functions must be quoted");
	std::string funcName = JSValueToStdString(ctx, argv[0]);
	int32_t ms = JSValueToInt32(ctx, argv[1]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action
	int player = QuickJS_GetInt32(ctx, global_obj, "me"); //engine->globalObject().property("me").toInt32();

	JSValue funcObj = JS_GetPropertyStr(ctx, global_obj, funcName.c_str()); //engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(ctx, JS_IsFunction(ctx, funcObj), "No such function: %s", funcName.c_str());
	JS_FreeValue(ctx, funcObj);

	std::string stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (argc == 3)
	{
		JSValue obj = argv[2];
		if (JS_IsString(obj))
		{
			stringArg = JSValueToStdString(ctx, obj);
		}
		else // is game object
		{
			int baseobj = QuickJS_GetInt32(ctx, obj, "id"); //obj.property("id").toInt32();
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, obj, "type"); //obj.property("type").toInt32();
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	scripting_engine::instance().setTimer(engineToInstanceMap.at(ctx)
	  // timerFunc
	, [ctx, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
		quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParams);
		std::vector<JSValue> args;
		if (baseObject != nullptr)
		{
			args.push_back(convMax(baseObject, ctx));
		}
		else if (pData && !(pData->stringArg.empty()))
		{
			args.push_back(JS_NewStringLen(ctx, pData->stringArg.c_str(), pData->stringArg.length()));
		}
		callFunction(ctx, funcName, args, true);
		std::for_each(args.begin(), args.end(), [ctx](JSValue& val) { JS_FreeValue(ctx, val); });
	}
	, player, ms, funcName, psObj, TIMER_REPEAT
	// additionalParams
	, new quickjs_timer_additionaldata(stringArg));

	return JS_TRUE;
}

//-- ## removeTimer(function)
//--
//-- Removes an existing timer. The first parameter is the function timer to remove,
//-- and its name _must be quoted_.
//--
static JSValue js_removeTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Timer functions must be quoted");
	std::string function = JSValueToStdString(ctx, argv[0]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action
	int player = QuickJS_GetInt32(ctx, global_obj, "me"); //engine->globalObject().property("me").toInt32();

	wzapi::scripting_instance* instance = engineToInstanceMap.at(ctx);
	std::vector<uniqueTimerID> removedTimerIDs = scripting_engine::instance().removeTimersIf(
		[instance, function, player](const scripting_engine::timerNode& node)
	{
		return (node.instance == instance) && (node.timerName == function) && (node.player == player);
	});
	if (removedTimerIDs.empty())
	{
		// Friendly warning
		std::string warnName = function;
		debug(LOG_ERROR, "Did not find timer %s to remove", warnName.c_str());
		return JS_FALSE;
	}
	return JS_TRUE;
}

//-- ## queue(function[, milliseconds[, object]])
//--
//-- Queues up a function to run at a later game frame. This is useful to prevent
//-- stuttering during the game, which can happen if too much script processing is
//-- done at once.  The function to run is the first parameter, and it
//-- _must be quoted_, otherwise the function will be inlined.
//-- The second parameter is the delay in milliseconds, if it is omitted or 0,
//-- the function will be run at a later frame.  A third optional
//-- parameter can be a **game object** to pass to the queued function. If the **game object**
//-- dies before the queued call runs, nothing happens.
//--
// TODO, check if an identical call is already queued up - and in this case,
// do not add anything.
static JSValue js_queue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc >= 1, "Must have at least one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Queued functions must be quoted");
	std::string funcName = JSValueToStdString(ctx, argv[0]);

	JSValue global_obj = JS_GetGlobalObject(ctx);
	auto free_global_obj = gsl::finally([ctx, global_obj] { JS_FreeValue(ctx, global_obj); });  // establish exit action

	JSValue funcObj = JS_GetPropertyStr(ctx, global_obj, funcName.c_str()); //engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(ctx, JS_IsFunction(ctx, funcObj), "No such function: %s", funcName.c_str());
	JS_FreeValue(ctx, funcObj);

	int32_t ms = 0;
	if (argc > 1)
	{
		ms = JSValueToInt32(ctx, argv[1]);
	}
	int player = QuickJS_GetInt32(ctx, global_obj, "me"); //int player = engine->globalObject().property("me").toInt32();

	std::string stringArg;
	BASE_OBJECT *psObj = nullptr;
	if (argc == 3)
	{
		JSValue obj = argv[2];
		if (JS_IsString(obj))
		{
			stringArg = JSValueToStdString(ctx, obj);
		}
		else // is game object
		{
			int baseobj = QuickJS_GetInt32(ctx, obj, "id"); //obj.property("id").toInt32();
			OBJECT_TYPE baseobjtype = (OBJECT_TYPE)QuickJS_GetInt32(ctx, obj, "type"); //obj.property("type").toInt32();
			psObj = IdToObject(baseobjtype, baseobj, player);
		}
	}

	scripting_engine::instance().setTimer(engineToInstanceMap.at(ctx)
	  // timerFunc
	, [ctx, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
		quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParams);
		std::vector<JSValue> args;
		if (baseObject != nullptr)
		{
			args.push_back(convMax(baseObject, ctx));
		}
		else if (pData && !(pData->stringArg.empty()))
		{
			args.push_back(JS_NewStringLen(ctx, pData->stringArg.c_str(), pData->stringArg.length()));
		}
		callFunction(ctx, funcName, args, true);
		std::for_each(args.begin(), args.end(), [ctx](JSValue& val) { JS_FreeValue(ctx, val); });
	}
	, player, ms, funcName, psObj, TIMER_ONESHOT_READY
	// additionalParams
	, new quickjs_timer_additionaldata(stringArg));
	return JS_TRUE;
}

//-- ## namespace(prefix)
//-- Registers a new event namespace. All events can now have this prefix. This is useful for
//-- code libraries, to implement event that do not conflict with events in main code. This
//-- function should be called from global; do not (for hopefully obvious reasons) put it
//-- inside an event.
//--
static JSValue js_namespace(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	SCRIPT_ASSERT(ctx, argc == 1, "Must have one parameter");
	SCRIPT_ASSERT(ctx, JS_IsString(argv[0]), "Must provide a string namespace prefix");
	std::string prefix = JSValueToStdString(ctx, argv[0]);
	engineToInstanceMap.at(ctx)->eventNamespaces.push_back(prefix);
	return JS_TRUE;
}

//ScriptMapData runMapScript_QtScript(WzString const &path, uint64_t seed, bool preview)
//{
//	ScriptMapData data;
//	data.valid = false;
//	data.mt = MersenneTwister(seed);
//
//	auto engine = std::unique_ptr<QScriptEngine>(new QScriptEngine());
//	//auto engine = std::make_unique<QScriptEngine>();
//	engine->setProcessEventsInterval(-1);
//	UDWORD size;
//	char *bytes = nullptr;
//	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
//	{
//		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
//		return data;
//	}
//	QString source = QString::fromUtf8(bytes, size);
//	free(bytes);
//	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
//	ASSERT_OR_RETURN(data, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
//	                 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
//	engine->globalObject().setProperty("preview", preview, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("XFLIP", TILE_XFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("YFLIP", TILE_YFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("ROTMASK", TILE_ROTMASK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("ROTSHIFT", TILE_ROTSHIFT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("TRIFLIP", TILE_TRIFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	//engine->globalObject().setProperty("players", players, QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("gameRand", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
//		auto &data = *(ScriptMapData *)_data;
//		uint32_t num = data.mt.u32();
//		uint32_t mod = context->argument(0).toUInt32();
//		return mod? num%mod : num;
//	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("log", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
//		auto &data = *(ScriptMapData *)_data;
//		(void)data;
//		auto str = context->argument(0).toString();
//		debug(LOG_INFO, "game.js: \"%s\"", str.toUtf8().constData());
//		return {};
//	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
//	engine->globalObject().setProperty("setMapData", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
//		auto &data = *(ScriptMapData *)_data;
//		data.valid = false;
//		auto mapWidth = context->argument(0);
//		auto mapHeight = context->argument(1);
//		auto texture = context->argument(2);
//		auto height = context->argument(3);
//		auto structures = context->argument(4);
//		auto droids = context->argument(5);
//		auto features = context->argument(6);
//		ASSERT_OR_RETURN(false, mapWidth.isNumber(), "mapWidth must be number");
//		ASSERT_OR_RETURN(false, mapHeight.isNumber(), "mapHeight must be number");
//		ASSERT_OR_RETURN(false, texture.isArray(), "texture must be array");
//		ASSERT_OR_RETURN(false, height.isArray(), "height must be array");
//		ASSERT_OR_RETURN(false, structures.isArray(), "structures must be array");
//		ASSERT_OR_RETURN(false, droids.isArray(), "droids must be array");
//		ASSERT_OR_RETURN(false, features.isArray(), "features must be array");
//		data.mapWidth = mapWidth.toInt32();
//		data.mapHeight = mapHeight.toInt32();
//		ASSERT_OR_RETURN(false, data.mapWidth > 1 && data.mapHeight > 1 && (uint64_t)data.mapWidth*data.mapHeight <= 65536, "Map size out of bounds");
//		size_t N = (size_t)data.mapWidth*data.mapHeight;
//		data.texture.resize(N);
//		data.height.resize(N);
//		for (size_t n = 0; n < N; ++n)
//		{
//			data.texture[n] = texture.property(n).toUInt16();
//			data.height[n] = height.property(n).toInt32();
//		}
//		uint16_t structureCount = structures.property("length").toUInt16();
//		for (unsigned i = 0; i < structureCount; ++i) {
//			auto structure = structures.property(i);
//			auto position = structure.property("position");
//			ScriptMapData::Structure sd;
//			sd.name = structure.property("name").toString().toUtf8().constData();
//			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
//			sd.direction = structure.property("direction").toInt32();
//			sd.modules = structure.property("modules").toUInt32();
//			sd.player = structure.property("player").toInt32();
//			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
//				ASSERT(false, "Invalid player");
//				continue;
//			}
//			data.structures.push_back(std::move(sd));
//		}
//		uint16_t droidCount = droids.property("length").toUInt16();
//		for (unsigned i = 0; i < droidCount; ++i) {
//			auto droid = droids.property(i);
//			auto position = droid.property("position");
//			ScriptMapData::Droid sd;
//			sd.name = droid.property("name").toString().toUtf8().constData();
//			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
//			sd.direction = droid.property("direction").toInt32();
//			sd.player = droid.property("player").toInt32();
//			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
//				ASSERT(false, "Invalid player");
//				continue;
//			}
//			data.droids.push_back(std::move(sd));
//		}
//		uint16_t featureCount = features.property("length").toUInt16();
//		for (unsigned i = 0; i < featureCount; ++i) {
//			auto feature = features.property(i);
//			auto position = feature.property("position");
//			ScriptMapData::Feature sd;
//			sd.name = feature.property("name").toString().toUtf8().constData();
//			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
//			sd.direction = feature.property("direction").toInt32();
//			data.features.push_back(std::move(sd));
//		}
//		data.valid = true;
//		return true;
//	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
//
//	QScriptValue result = engine->evaluate(source, QString::fromUtf8(path.toUtf8().c_str()));
//	ASSERT_OR_RETURN(data, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s",
//	                 engine->uncaughtExceptionLineNumber(), path.toUtf8().c_str(), result.toString().toUtf8().constData());
//
//	return data;
//}

wzapi::scripting_instance* createQuickJSScriptInstance(const WzString& path, int player, int difficulty)
{
	QFileInfo basename(QString::fromUtf8(path.toUtf8().c_str()));
	quickjs_scripting_instance* pNewInstance = new quickjs_scripting_instance(player, basename.baseName().toStdString());
	if (!pNewInstance->loadScript(path, player, difficulty))
	{
		delete pNewInstance;
		return nullptr;
	}
	return pNewInstance;
}

bool QuickJS_EnumerateObjectProperties(JSContext *ctx, JSValue obj, const std::function<void (const char *key, JSAtom& atom)>& func)
{
	ASSERT_OR_RETURN(false, JS_IsObject(obj), "obj is not a JS object");
	JSPropertyEnum * properties = nullptr;
    uint32_t count = 0;
	int32_t flags = (JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK | JS_GPN_ENUM_ONLY);
	if (JS_GetOwnPropertyNames(ctx, &properties, &count, obj, flags) != 0)
	{
		// JS_GetOwnPropertyNames failed?
		return false;
	}

	for(uint32_t i = 0; i < count; i++)
    {
        JSAtom atom = properties[i].atom;

        const char *key = JS_AtomToCString(ctx, atom);

		func(key, atom);

//        JSValue jsval = JS_GetProperty(ctx, obj, atom);
//        const char *val = JS_ToCString(ctx, jsval);
//
//        fprintf(stderr, "key=%s, val=%s\n", key, val);

//        JS_FreeValue(ctx, jsval);
        JS_FreeCString(ctx, key);
//        JS_FreeCString(ctx, val);
    }
	for (int i = 0; i < count; i++)
	{
		JS_FreeAtom(ctx, properties[i].atom);
	}
	js_free(ctx, properties);
	return true;
}

bool quickjs_scripting_instance::loadScript(const WzString& path, int player, int difficulty)
{
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return false;
	}
	m_path = path.toUtf8();
	compiledScriptObj = JS_Eval(ctx, bytes, size, path.toUtf8().c_str(), JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
	free(bytes);
	if (JS_IsException(compiledScriptObj))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
//		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Syntax / compilation error in %s: %s",
			  path.toUtf8().c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, compiledScriptObj);
		compiledScriptObj = JS_UNINITIALIZED;
		return false;
	}
//	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(m_source);
//	ASSERT_OR_RETURN(false, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
//					 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());

	// Special functions
	static const JSCFunctionListEntry js_builtin_funcs[] = {
		JS_CFUNC_DEF("setTimer", 2, js_setTimer ), // JS-specific implementation
		JS_CFUNC_DEF("queue", 1, js_queue ), // JS-specific implementation
		JS_CFUNC_DEF("removeTimer", 1, js_removeTimer ), // JS-specific implementation
		JS_CFUNC_DEF("profile", 1, js_profile ), // JS-specific implementation
		JS_CFUNC_DEF("include", 1, js_include ), // backend-specific (a scripting_instance can't directly include a different type of script)
		JS_CFUNC_DEF("namespace", 1, js_namespace ) // JS-specific implementation
	};
	JS_SetPropertyFunctionList(ctx, global_obj, js_builtin_funcs, sizeof(js_builtin_funcs) / sizeof(js_builtin_funcs[0]));
//	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer)); // JS-specific implementation
//	engine->globalObject().setProperty("queue", engine->newFunction(js_queue)); // JS-specific implementation
//	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer)); // JS-specific implementation
//	engine->globalObject().setProperty("profile", engine->newFunction(js_profile)); // JS-specific implementation
//	engine->globalObject().setProperty("include", engine->newFunction(js_include)); // backend-specific (a scripting_instance can't directly include a different type of script)
//	engine->globalObject().setProperty("namespace", engine->newFunction(js_namespace)); // JS-specific implementation

	// Regular functions
	QFileInfo basename(QString::fromUtf8(path.toUtf8().c_str()));
	registerFunctions(basename.baseName().toStdString());

	// Remember internal, reserved names
	std::unordered_set<std::string>& internalNamespaceRef = internalNamespace;
	QuickJS_EnumerateObjectProperties(ctx, global_obj, [&internalNamespaceRef](const char *key, JSAtom &) {
		internalNamespaceRef.insert(key);
	});

	return true;
}

bool quickjs_scripting_instance::readyInstanceForExecution()
{
	JSValue result = JS_EvalFunction(ctx, compiledScriptObj);
	compiledScriptObj = JS_UNINITIALIZED;
	if (JS_IsException(result))
	{
		// compilation error / syntax error
		std::string errorAsString = QuickJS_DumpError(ctx);
//		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Syntax / compilation error in %s: %s",
			  m_path.c_str(), errorAsString.c_str());
		JS_FreeValue(ctx, result);
		return false;
	}

	return true;
}

bool quickjs_scripting_instance::saveScriptGlobals(nlohmann::json &result)
{
	// we save 'scriptName' and 'me' implicitly
	QuickJS_EnumerateObjectProperties(ctx, global_obj, [this, &result](const char *key, JSAtom &atom) {
        JSValue jsVal = JS_GetProperty(ctx, global_obj, atom);
		std::string nameStr = key;
		if (internalNamespace.count(nameStr) == 0 && !JS_IsFunction(ctx, jsVal)
			)//&& !it.value().equals(engine->globalObject()))
		{
			result[nameStr] = JSContextValue{ctx, jsVal};
		}
        JS_FreeValue(ctx, jsVal);
	});
	return true;
}

bool quickjs_scripting_instance::loadScriptGlobals(const nlohmann::json &result)
{
	ASSERT_OR_RETURN(false, result.is_object(), "Can't load script globals from non-json-object");
	for (auto it : result.items())
	{
		// IMPORTANT: "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//			  (mapJsonToQuickJSValue handles this properly.)
		JS_DefinePropertyValueStr(ctx, global_obj, it.key().c_str(), mapJsonToQuickJSValue(ctx, it.value(), JS_PROP_C_W_E), JS_PROP_C_W_E); // TODO: Which flag to pass?
//		engine->globalObject().setProperty(QString::fromStdString(it.key()), mapJsonToQScriptValue(ctx, it.value(), QScriptValue::PropertyFlags()));
	}
	return true;
}

nlohmann::json quickjs_scripting_instance::saveTimerFunction(uniqueTimerID timerID, std::string timerName, timerAdditionalData* additionalParam)
{
	nlohmann::json result = nlohmann::json::object();
	result["function"] = timerName;
	quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParam);
	if (pData)
	{
		result["stringArg"] = pData->stringArg;
	}
	return result;
}

// recreates timer functions (and additional userdata) based on the information saved by the saveTimer() method
std::tuple<TimerFunc, timerAdditionalData *> quickjs_scripting_instance::restoreTimerFunction(const nlohmann::json& savedTimerFuncData)
{
	std::string funcName = json_getValue(savedTimerFuncData, WzString::fromUtf8("function")).toWzString().toStdString();
	if (funcName.empty())
	{
		throw std::runtime_error("Invalid timer restore data");
	}
	std::string stringArg = json_getValue(savedTimerFuncData, WzString::fromUtf8("stringArg")).toWzString().toStdString();

	JSContext* pContext = ctx;

	return std::tuple<TimerFunc, timerAdditionalData *>{
		// timerFunc
		[pContext, funcName](uniqueTimerID timerID, BASE_OBJECT* baseObject, timerAdditionalData* additionalParams) {
			quickjs_timer_additionaldata* pData = static_cast<quickjs_timer_additionaldata*>(additionalParams);
			std::vector<JSValue> args;
			if (baseObject != nullptr)
			{
				args.push_back(convMax(baseObject, pContext));
			}
			else if (pData && !(pData->stringArg.empty()))
			{
				args.push_back(JS_NewStringLen(pContext, pData->stringArg.c_str(), pData->stringArg.length()));
			}
			callFunction(pContext, funcName, args, true);
			std::for_each(args.begin(), args.end(), [pContext](JSValue& val) { JS_FreeValue(pContext, val); });
		}
		// additionalParams
		, new quickjs_timer_additionaldata(stringArg)
	};
}

nlohmann::json quickjs_scripting_instance::debugGetAllScriptGlobals()
{
	nlohmann::json globals = nlohmann::json::object();
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		if ((internalNamespace.count(it.name().toStdString()) == 0 && !it.value().isFunction()
			 && !it.value().equals(engine->globalObject()))
			|| it.name() == "Upgrades" || it.name() == "Stats")
		{
			globals[it.name().toStdString()] = (it.value().toVariant()); // uses to_json QVariant implementation
		}
	}
	return globals;
}

bool quickjs_scripting_instance::debugEvaluateCommand(const std::string &_text)
{
	QString text = QString::fromStdString(_text);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(text);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in %s: %s",
		      text.toUtf8().constData(), syntax.errorMessage().toUtf8().constData());
		return false;
	}
	QScriptValue result = engine->evaluate(text);
	if (engine->hasUncaughtException())
	{
		debug(LOG_ERROR, "Uncaught exception in %s: %s",
		      text.toUtf8().constData(), result.toString().toUtf8().constData());
		return false;
	}
	console("%s", result.toString().toUtf8().constData());
	return true;
}

void quickjs_scripting_instance::updateGameTime(uint32_t gameTime)
{
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

void quickjs_scripting_instance::updateGroupSizes(int groupId, int size)
{
	QScriptValue groupMembers = engine->globalObject().property("groupSizes");
	groupMembers.setProperty(groupId, size, QScriptValue::ReadOnly);
}

void quickjs_scripting_instance::setSpecifiedGlobalVariables(const nlohmann::json& variables, wzapi::GlobalVariableFlags flags /*= wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave*/)
{
	if (!variables.is_object())
	{
		ASSERT(false, "setSpecifiedGlobalVariables expects a JSON object");
		return;
	}
	QScriptValue::PropertyFlags propertyFlags = QScriptValue::Undeletable;
	if ((flags & wzapi::GlobalVariableFlags::ReadOnly) == wzapi::GlobalVariableFlags::ReadOnly)
	{
		propertyFlags |= QScriptValue::ReadOnly;
	}
	bool markGlobalAsInternal = (flags & wzapi::GlobalVariableFlags::DoNotSave) == wzapi::GlobalVariableFlags::DoNotSave;
	for (auto it : variables.items())
	{
		ASSERT(!it.key().empty(), "Empty key");
		engine->globalObject().setProperty(
			QString::fromStdString(it.key()),
			mapJsonToQScriptValue(engine, it.value(), propertyFlags),
			propertyFlags
		);
		if (markGlobalAsInternal)
		{
			internalNamespace.insert(it.key());
		}
	}
}

void quickjs_scripting_instance::doNotSaveGlobal(const std::string &global)
{
	internalNamespace.insert(global);
}


IMPL_EVENT_HANDLER_NO_PARAMS(eventGameInit)
IMPL_EVENT_HANDLER_NO_PARAMS(eventStartLevel)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMissionTimeout)
IMPL_EVENT_HANDLER_NO_PARAMS(eventVideoDone)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameLoaded)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameSaving)
IMPL_EVENT_HANDLER_NO_PARAMS(eventGameSaved)

// MARK: Transporter events
IMPL_EVENT_HANDLER_NO_PARAMS(eventLaunchTransporter) // DEPRECATED!
IMPL_EVENT_HANDLER(eventTransporterLaunch, const BASE_OBJECT *)
IMPL_EVENT_HANDLER_NO_PARAMS(eventReinforcementsArrived) // DEPRECATED!
IMPL_EVENT_HANDLER(eventTransporterArrived, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterExit, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterDone, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventTransporterLanded, const BASE_OBJECT *)

// MARK: UI-related events (intended for the tutorial)
IMPL_EVENT_HANDLER(eventDeliveryPointMoving, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventDeliveryPointMoved, const BASE_OBJECT *)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignBody)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignPropulsion)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignWeapon)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignCommand)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignSystem)
IMPL_EVENT_HANDLER_NO_PARAMS(eventDesignQuit)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuBuildSelected)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuResearchSelected)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuBuild)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuResearch)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuDesign)
IMPL_EVENT_HANDLER_NO_PARAMS(eventMenuManufacture)
IMPL_EVENT_HANDLER(eventSelectionChanged, const std::vector<const BASE_OBJECT *>&)

// MARK: Game state-change events
IMPL_EVENT_HANDLER(eventObjectRecycled, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventPlayerLeft, int)
IMPL_EVENT_HANDLER(eventCheatMode, bool)
IMPL_EVENT_HANDLER(eventDroidIdle, const DROID *)
IMPL_EVENT_HANDLER(eventDroidBuilt, const DROID *, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventStructBuilt, const STRUCTURE *, const DROID *)
IMPL_EVENT_HANDLER(eventStructDemolish, const STRUCTURE *, const DROID *)
IMPL_EVENT_HANDLER(eventStructureReady, const STRUCTURE *)
IMPL_EVENT_HANDLER(eventAttacked, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventResearched, const wzapi::researchResult&, const STRUCTURE *, int)
IMPL_EVENT_HANDLER(eventDestroyed, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventPickup, const FEATURE *, const DROID *)
IMPL_EVENT_HANDLER(eventObjectSeen, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventGroupSeen, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventObjectTransfer, const BASE_OBJECT *, int)
IMPL_EVENT_HANDLER(eventChat, int, int, const char *)
IMPL_EVENT_HANDLER(eventBeacon, int, int, int, int, const char *)
IMPL_EVENT_HANDLER(eventBeaconRemoved, int, int)
IMPL_EVENT_HANDLER(eventGroupLoss, const BASE_OBJECT *, int, int)
bool qtscript_scripting_instance::handle_eventArea(const std::string& label, const DROID *psDroid)
{
	QScriptValueList args;
	args += convDroid(psDroid, engine);
	QString funcname = QString("eventArea" + QString::fromStdString(label));
	debug(LOG_SCRIPT, "Triggering %s for %s", funcname.toUtf8().constData(),
		  engine->globalObject().property("scriptName").toString().toUtf8().constData());
	callFunction(engine, funcname, args);
	return true;
}
IMPL_EVENT_HANDLER(eventDesignCreated, const DROID_TEMPLATE *)
IMPL_EVENT_HANDLER(eventAllianceOffer, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceAccepted, uint8_t, uint8_t)
IMPL_EVENT_HANDLER(eventAllianceBroken, uint8_t, uint8_t)

// MARK: Special input events
IMPL_EVENT_HANDLER(eventSyncRequest, int, int, int, int, const BASE_OBJECT *, const BASE_OBJECT *)
IMPL_EVENT_HANDLER(eventKeyPressed, int, int)

// ----------------------------------------------------------------------------------------
// Script functions
//
// All script functions should be prefixed with "js_" then followed by same name as in script.

//-- ## getWeaponInfo(weapon id)
//--
//-- Return information about a particular weapon type. DEPRECATED - query the Stats object instead. (3.2+ only)
//--
static QScriptValue js_getWeaponInfo(QScriptContext *context, QScriptEngine *engine)
{
	QString id = context->argument(0).toString();
	int idx = getCompFromName(COMP_WEAPON, QStringToWzString(id));
	SCRIPT_ASSERT(context, idx >= 0, "No such weapon: %s", id.toUtf8().constData());
	WEAPON_STATS *psStats = asWeaponStats + idx;
	QScriptValue info = engine->newObject();
	info.setProperty("id", id);
	info.setProperty("name", WzStringToQScriptValue(engine, psStats->name));
	info.setProperty("impactClass", psStats->weaponClass == WC_KINETIC ? "KINETIC" : "HEAT");
	info.setProperty("damage", psStats->base.damage);
	info.setProperty("firePause", psStats->base.firePause);
	info.setProperty("fireOnMove", psStats->fireOnMove);
	return QScriptValue(info);
}

//-- ## resetLabel(label[, filter])
//--
//-- Reset the trigger on an label. Next time a unit enters the area, it will trigger
//-- an area event. Next time an object or a group is seen, it will trigger a seen event.
//-- Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ALL_PLAYERS by default.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//-- ## resetArea(label[, filter])
//-- Reset the trigger on an area. Next time a unit enters the area, it will trigger
//-- an area event. Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ALL_PLAYERS by default.
//-- This is a fast operation of O(log n) algorithmic complexity. DEPRECATED - use resetLabel instead. (3.2+ only)
//--
static QScriptValue js_resetLabel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::resetLabel, context, engine);
}

//-- ## enumLabels([filter])
//--
//-- Returns a string list of labels that exist for this map. The optional filter
//-- parameter can be used to only return labels of one specific type. (3.2+ only)
//--
static QScriptValue js_enumLabels(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::enumLabels, context, engine);
}

//-- ## addLabel(object, label)
//--
//-- Add a label to a game object. If there already is a label by that name, it is overwritten.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_addLabel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::addLabel, context, engine);
}

//-- ## removeLabel(label)
//--
//-- Remove a label from the game. Returns the number of labels removed, which should normally be
//-- either 1 (label found) or 0 (label not found). (3.2+ only)
//--
static QScriptValue js_removeLabel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::removeLabel, context, engine);
}

//-- ## getLabel(object)
//--
//-- Get a label string belonging to a game object. If the object has multiple labels, only the first
//-- label found will be returned. If the object has no labels, null is returned.
//-- This is a relatively slow operation of O(n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_getLabel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::getLabelJS, context, engine);
}

//-- ## getObject(label | x, y | type, player, id)
//--
//-- Fetch something denoted by a label, a map position or its object ID. A label refers to an area,
//-- a position or a **game object** on the map defined using the map editor and stored
//-- together with the map. In this case, the only argument is a text label. The function
//-- returns an object that has a type variable defining what it is (in case this is
//-- unclear). This type will be one of DROID, STRUCTURE, FEATURE, AREA, GROUP or POSITION.
//-- The AREA has defined 'x', 'y', 'x2', and 'y2', while POSITION has only defined 'x' and 'y'.
//-- The GROUP type has defined 'type' and 'id' of the group, which can be passed to enumGroup().
//-- This is a fast operation of O(log n) algorithmic complexity. If the label is not found, an
//-- undefined value is returned. If whatever object the label should point at no longer exists,
//-- a null value is returned.
//--
//-- You can also fetch a STRUCTURE or FEATURE type game object from a given map position (if any).
//-- This is a very fast operation of O(1) algorithmic complexity. Droids cannot be fetched in this
//-- manner, since they do not have a unique placement on map tiles. Finally, you can fetch an object using
//-- its ID, in which case you need to pass its type, owner and unique object ID. This is an
//-- operation of O(n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_getObject(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::getObject, context, engine);
}

//-- ## enumBlips(player)
//--
//-- Return an array containing all the non-transient radar blips that the given player
//-- can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
//-- It does not include units going out of view.
//--
static QScriptValue js_enumBlips(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumBlips, context, engine);
}

//-- ## enumSelected()
//--
//-- Return an array containing all game objects currently selected by the host player. (3.2+ only)
//--
QScriptValue js_enumSelected(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumSelected, context, engine);
}

//-- ## enumGateways()
//--
//-- Return an array containing all the gateways on the current map. The array contains object with the properties
//-- x1, y1, x2 and y2. (3.2+ only)
//--
static QScriptValue js_enumGateways(QScriptContext *, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(gwNumGateways());
	int i = 0;
	for (auto psGateway : gwGetGateways())
	{
		QScriptValue v = engine->newObject();
		v.setProperty("x1", psGateway->x1, QScriptValue::ReadOnly);
		v.setProperty("y1", psGateway->y1, QScriptValue::ReadOnly);
		v.setProperty("x2", psGateway->x2, QScriptValue::ReadOnly);
		v.setProperty("y2", psGateway->y2, QScriptValue::ReadOnly);
		result.setProperty(i++, v);
	}
	return result;
}

//-- ## enumTemplates(player)
//--
//-- Return an array containing all the buildable templates for the given player. (3.2+ only)
//--
static QScriptValue js_enumTemplates(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	QScriptValue result = engine->newArray(droidTemplates[player].size());
	int count = 0;
	for (auto &keyvaluepair : droidTemplates[player])
	{
		result.setProperty(count, convTemplate(keyvaluepair.second, engine));
		count++;
	}
	return result;
}

//-- ## enumGroup(group)
//--
//-- Return an array containing all the members of a given group.
//--
static QScriptValue js_enumGroup(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::enumGroup, context, engine);
}

//-- ## newGroup()
//--
//-- Allocate a new group. Returns its numerical ID. Deprecated since 3.2 - you should now
//-- use your own number scheme for groups.
//--
static QScriptValue js_newGroup(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::newGroup, context, engine);
}

//-- ## activateStructure(structure, [target[, ability]])
//--
//-- Activate a special ability on a structure. Currently only works on the lassat.
//-- The lassat needs a target.
//--
static QScriptValue js_activateStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::activateStructure, context, engine);
}

//-- ## findResearch(research, [player])
//--
//-- Return list of research items remaining to be researched for the given research item. (3.2+ only)
//-- (Optional second argument 3.2.3+ only)
//--
static QScriptValue js_findResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::findResearch, context, engine);
}

//-- ## pursueResearch(lab, research)
//--
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
//--
static QScriptValue js_pursueResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::pursueResearch, context, engine);
}

//-- ## getResearch(research[, player])
//--
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.json". If not found, returns null.
//--
static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getResearch, context, engine);
}

//-- ## enumResearch()
//--
//-- Returns an array of all research objects that are currently and immediately available for research.
//--
static QScriptValue js_enumResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumResearch, context, engine);
}

//-- ## componentAvailable([component type,] component name)
//--
//-- Checks whether a given component is available to the current player. The first argument is
//-- optional and deprecated.
//--
static QScriptValue js_componentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::componentAvailable, context, engine);
}

//-- ## addFeature(name, x, y)
//--
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
//--
static QScriptValue js_addFeature(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addFeature, context, engine);
}

//-- ## addDroid(player, x, y, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Create and place a droid at the given x, y position as belonging to the given player, built with
//-- the given components. Currently does not support placing droids in multiplayer, doing so will
//-- cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
//-- reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
//-- pass -1 as both x and y.
//--
static QScriptValue js_addDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addDroid, context, engine);
}

//-- ## addDroidToTransporter(transporter, droid)
//--
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
//--
static QScriptValue js_addDroidToTransporter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addDroidToTransporter, context, engine);
}

//-- ## makeTemplate(player, name, body, propulsion, reserved, turrets...)
//--
//-- Create a template (virtual droid) with the given components. Can be useful for calculating the cost
//-- of droids before putting them into production, for instance. Will fail and return null if template
//-- could not possibly be built using current research. (3.2+ only)
//--
static QScriptValue js_makeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::makeTemplate, context, engine);
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
static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::buildDroid, context, engine);
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
static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumStruct, context, engine);
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
static QScriptValue js_enumStructOffWorld(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumStructOffWorld, context, engine);
}

//-- ## enumFeature(player[, name])
//--
//-- Returns an array of all features seen by player of given name, as defined in "features.json".
//-- If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
//--
static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumFeature, context, engine);
}

//-- ## enumCargo(transport droid)
//--
//-- Returns an array of droid objects inside given transport. (3.2+ only)
//--
static QScriptValue js_enumCargo(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumCargo, context, engine);
}

//-- ## enumDroid([player[, droid type[, looking player]]])
//--
//-- Returns an array of droid objects. If no parameters given, it will
//-- return all of the droids for the current player. The second, optional parameter
//-- is the name of the droid type. The third parameter can be used to filter by
//-- visibility - the default is not to filter.
//--
static QScriptValue js_enumDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumDroid, context, engine);
}

void dumpScriptLog(const WzString &scriptName, int me, const std::string &info)
{
	WzString path = PHYSFS_getWriteDir();
	path += WzString("/logs/") + scriptName + "." + WzString::number(me) + ".log";
	FILE *fp = fopen(path.toUtf8().c_str(), "a"); // TODO: This will fail for unicode paths on Windows. Should use PHYSFS to open / write files
	if (fp)
	{
		fputs(info.c_str(), fp);
		fclose(fp);
	}
}

//-- ## dump(string...)
//--
//-- Output text to a debug file. (3.2+ only)
//--
static QScriptValue js_dump(QScriptContext *context, QScriptEngine *engine)
{
	std::string result;
	for (int i = 0; i < context->argumentCount(); ++i)
	{
		if (i != 0)
		{
			result.append(" ");
		}
		QString qStr = context->argument(i).toString();
		if (context->state() == QScriptContext::ExceptionState)
		{
			break;
		}
		result.append(qStr.toStdString());
	}
	result += "\n";

	WzString scriptName = QStringToWzString(engine->globalObject().property("scriptName").toString());
	int me = engine->globalObject().property("me").toInt32();
	dumpScriptLog(scriptName, me, result);
	return QScriptValue();
}

//-- ## debug(string...)
//--
//-- Output text to the command line.
//--
static QScriptValue js_debug(QScriptContext *context, QScriptEngine *engine)
{
	QString result;
	for (int i = 0; i < context->argumentCount(); ++i)
	{
		if (i != 0)
		{
			result.append(QLatin1String(" "));
		}
		QString s = context->argument(i).toString();
		if (context->state() == QScriptContext::ExceptionState)
		{
			break;
		}
		result.append(s);
	}
	qWarning("%s", result.toUtf8().constData());
	return QScriptValue();
}

//-- ## pickStructLocation(droid, structure type, x, y)
//--
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
//--
static QScriptValue js_pickStructLocation(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::pickStructLocation, context, engine);
}

//-- ## structureIdle(structure)
//--
//-- Is given structure idle?
//--
static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::structureIdle, context, engine);
}

//-- ## removeStruct(structure)
//--
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2.
//--
static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeStruct, context, engine);
}

//-- ## removeObject(game object[, special effects?])
//--
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
//--
static QScriptValue js_removeObject(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeObject, context, engine);
}

//-- ## clearConsole()
//--
//-- Clear the console. (3.3+ only)
//--
static QScriptValue js_clearConsole(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::clearConsole, context, engine);
}

//-- ## console(strings...)
//--
//-- Print text to the player console.
//--
// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::console, context, engine);
}

//-- ## groupAddArea(group, x1, y1, x2, y2)
//--
//-- Add any droids inside the given area to the given group. (3.2+ only)
//--
static QScriptValue js_groupAddArea(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::groupAddArea, context, engine);
}

//-- ## groupAddDroid(group, droid)
//--
//-- Add given droid to given group. Deprecated since 3.2 - use groupAdd() instead.
//--
static QScriptValue js_groupAddDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::groupAddDroid, context, engine);
}

//-- ## groupAdd(group, object)
//--
//-- Add given game object to the given group.
//--
static QScriptValue js_groupAdd(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::groupAdd, context, engine);
}

//-- ## distBetweenTwoPoints(x1, y1, x2, y2)
//--
//-- Return distance between two points.
//--
static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::distBetweenTwoPoints, context, engine);
}

//-- ## groupSize(group)
//--
//-- Return the number of droids currently in the given group. Note that you can use groupSizes[] instead.
//--
static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::groupSize, context, engine);
}

//-- ## droidCanReach(droid, x, y)
//--
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
//--
static QScriptValue js_droidCanReach(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::droidCanReach, context, engine);
}

//-- ## propulsionCanReach(propulsion, x1, y1, x2, y2)
//--
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
//--
static QScriptValue js_propulsionCanReach(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::propulsionCanReach, context, engine);
}

//-- ## terrainType(x, y)
//--
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
//--
static QScriptValue js_terrainType(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::terrainType, context, engine);
}

//-- ## orderDroid(droid, order)
//--
//-- Give a droid an order to do something. (3.2+ only)
//--
static QScriptValue js_orderDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroid, context, engine);
}

//-- ## orderDroidObj(droid, order, object)
//--
//-- Give a droid an order to do something to something.
//--
static QScriptValue js_orderDroidObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidObj, context, engine);
}

//-- ## orderDroidBuild(droid, order, structure type, x, y[, direction])
//--
//-- Give a droid an order to build something at the given position. Returns true if allowed.
//--
static QScriptValue js_orderDroidBuild(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidBuild, context, engine);
}

//-- ## orderDroidLoc(droid, order, x, y)
//--
//-- Give a droid an order to do something at the given location.
//--
static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidLoc, context, engine);
}

//-- ## setMissionTime(time)
//--
//-- Set mission countdown in seconds.
//--
static QScriptValue js_setMissionTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setMissionTime, context, engine);
}

//-- ## getMissionTime()
//--
//-- Get time remaining on mission countdown in seconds. (3.2+ only)
//--
static QScriptValue js_getMissionTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getMissionTime, context, engine);
}

//-- ## setTransporterExit(x, y, player)
//--
//-- Set the exit position for the mission transporter. (3.2+ only)
//--
static QScriptValue js_setTransporterExit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setTransporterExit, context, engine);
}

//-- ## startTransporterEntry(x, y, player)
//--
//-- Set the entry position for the mission transporter, and make it start flying in
//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
//-- The transport needs to be set up with the mission droids, and the first transport
//-- found will be used. (3.2+ only)
//--
static QScriptValue js_startTransporterEntry(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::startTransporterEntry, context, engine);
}

//-- ## useSafetyTransport(flag)
//--
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.3+ only).
//--
static QScriptValue js_useSafetyTransport(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::useSafetyTransport, context, engine);
}

//-- ## restoreLimboMissionData()
//--
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.3+ only).
//--
static QScriptValue js_restoreLimboMissionData(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::restoreLimboMissionData, context, engine);
}

//-- ## setReinforcementTime(time)
//--
//-- Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
//-- is removed and the timer stopped. Time is in seconds.
//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
//-- is set to "--:--" and reinforcements are suppressed until this function is called
//-- again with a regular time value.
//--
static QScriptValue js_setReinforcementTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReinforcementTime, context, engine);
}

//-- ## setStructureLimits(structure type, limit[, player])
//--
//-- Set build limits for a structure.
//--
static QScriptValue js_setStructureLimits(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setStructureLimits, context, engine);
}

//-- ## centreView(x, y)
//--
//-- Center the player's camera at the given position.
//--
static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::centreView, context, engine);
}

//-- ## hackPlayIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackPlayIngameAudio(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackPlayIngameAudio, context, engine);
}

//-- ## hackStopIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackStopIngameAudio(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackStopIngameAudio, context, engine);
}

//-- ## playSound(sound[, x, y, z])
//--
//-- Play a sound, optionally at a location.
//--
static QScriptValue js_playSound(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::playSound, context, engine);
}

//-- ## gameOverMessage(won, showBackDrop, showOutro)
//--
//-- End game in victory or defeat.
//--
static QScriptValue js_gameOverMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::gameOverMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## completeResearch(research[, player [, forceResearch]])
//--
//-- Finish a research for the given player.
//-- forceResearch will allow a research topic to be researched again. 3.3+
//--
static QScriptValue js_completeResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::completeResearch, context, engine);
}

//-- ## completeAllResearch([player])
//--
//-- Finish all researches for the given player.
//--
static QScriptValue js_completeAllResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::completeAllResearch, context, engine);
}

//-- ## enableResearch(research[, player])
//--
//-- Enable a research for the given player, allowing it to be researched.
//--
static QScriptValue js_enableResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableResearch, context, engine);
}

//-- ## extraPowerTime(time, player)
//--
//-- Increase a player's power as if that player had power income equal to current income
//-- over the given amount of extra time. (3.2+ only)
//--
static QScriptValue js_extraPowerTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::extraPowerTime, context, engine);
}

//-- ## setPower(power[, player])
//--
//-- Set a player's power directly. (Do not use this in an AI script.)
//--
static QScriptValue js_setPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPower, context, engine);
}

//-- ## setPowerModifier(power[, player])
//--
//-- Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPowerModifier, context, engine);
}

//-- ## setPowerStorageMaximum(maximum[, player])
//--
//-- Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerStorageMaximum(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPowerStorageMaximum, context, engine);
}

//-- ## enableStructure(structure type[, player])
//--
//-- The given structure type is made available to the given player. It will appear in the
//-- player's build list.
//--
static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableStructure, context, engine);
}

//-- ## setTutorialMode(bool)
//--
//-- Sets a number of restrictions appropriate for tutorial if set to true.
//--
static QScriptValue js_setTutorialMode(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setTutorialMode, context, engine);
}

//-- ## setMiniMap(bool)
//--
//-- Turns visible minimap on or off in the GUI.
//--
static QScriptValue js_setMiniMap(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setMiniMap, context, engine);
}

//-- ## setDesign(bool)
//--
//-- Whether to allow player to design stuff.
//--
static QScriptValue js_setDesign(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDesign, context, engine);
}

//-- ## enableTemplate(template name)
//--
//-- Enable a specific template (even if design is disabled).
//--
static QScriptValue js_enableTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableTemplate, context, engine);
}

//-- ## removeTemplate(template name)
//--
//-- Remove a template.
//--
static QScriptValue js_removeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeTemplate, context, engine);
}

//-- ## setReticuleButton(id, tooltip, filename, filenameHigh, callback)
//--
//-- Add reticule button. id is which button to change, where zero is zero is the middle button, then going clockwise from the
//-- uppermost button. filename is button graphics and filenameHigh is for highlighting. The tooltip is the text you see when
//-- you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
//-- for such changes to take effect. (3.2+ only)
//--
static QScriptValue js_setReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReticuleButton, context, engine);
}

//-- ## showReticuleWidget(id)
//--
//-- Open the reticule menu widget. (3.3+ only)
//--
static QScriptValue js_showReticuleWidget(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::showReticuleWidget, context, engine);
}

//-- ## setReticuleFlash(id, flash)
//--
//-- Set reticule flash on or off. (3.2.3+ only)
//--
static QScriptValue js_setReticuleFlash(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReticuleFlash, context, engine);
}

//-- ## showInterface()
//--
//-- Show user interface. (3.2+ only)
//--
static QScriptValue js_showInterface(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::showInterface, context, engine);
}

//-- ## hideInterface()
//--
//-- Hide user interface. (3.2+ only)
//--
static QScriptValue js_hideInterface(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hideInterface, context, engine);
}

//-- ## removeReticuleButton(button type)
//--
//-- Remove reticule button. DO NOT USE FOR ANYTHING.
//--
static QScriptValue js_removeReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
}

//-- ## applyLimitSet()
//--
//-- Mix user set limits with script set limits and defaults.
//--
static QScriptValue js_applyLimitSet(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::applyLimitSet, context, engine);
}

//-- ## enableComponent(component, player)
//--
//-- The given component is made available for research for the given player.
//--
static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableComponent, context, engine);
}

//-- ## makeComponentAvailable(component, player)
//--
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
//--
static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::makeComponentAvailable, context, engine);
}

//-- ## allianceExistsBetween(player, player)
//--
//-- Returns true if an alliance exists between the two players, or they are the same player.
//--
static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::allianceExistsBetween, context, engine);
}

//-- ## _(string)
//--
//-- Mark string for translation.
//--
static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::translate, context, engine);
}

//-- ## playerPower(player)
//--
//-- Return amount of power held by the given player.
//--
static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::playerPower, context, engine);
}

//-- ## queuedPower(player)
//--
//-- Return amount of power queued up for production by the given player. (3.2+ only)
//--
static QScriptValue js_queuedPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::queuedPower, context, engine);
}

//-- ## isStructureAvailable(structure type[, player])
//--
//-- Returns true if given structure can be built. It checks both research and unit limits.
//--
static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::isStructureAvailable, context, engine);
}

//-- ## isVTOL(droid)
//--
//-- Returns true if given droid is a VTOL (not including transports).
//--
static QScriptValue js_isVTOL(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::isVTOL, context, engine);
}

//-- ## hackGetObj(type, player, id)
//--
//-- Function to find and return a game object of DROID, FEATURE or STRUCTURE types, if it exists.
//-- Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)
//--
static QScriptValue js_hackGetObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackGetObj, context, engine);
}

//-- ## hackChangeMe(player)
//--
//-- Change the 'me' who owns this script to the given player. This needs to be run
//-- first in ```eventGameInit``` to make sure things do not get out of control.
//--
// This is only intended for use in campaign scripts until we get a way to add
// scripts for each player. (3.2+ only)
static QScriptValue js_hackChangeMe(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackChangeMe, context, engine);
}

//-- ## receiveAllEvents(bool)
//--
//-- Make the current script receive all events, even those not meant for 'me'. (3.2+ only)
//--
static QScriptValue js_receiveAllEvents(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::receiveAllEvents, context, engine);
}

//-- ## hackAssert(condition, message...)
//--
//-- Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)
//--
static QScriptValue js_hackAssert(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackAssert, context, engine);
}

//-- ## objFromId(fake game object)
//--
//-- Broken function meant to make porting from the old scripting system easier. Do not use for new code.
//-- Instead, use labels.
//--
static QScriptValue js_objFromId(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	BASE_OBJECT *psObj = getBaseObjFromId(id);
	SCRIPT_ASSERT(context, psObj, "No such object id %d", id);
	return QScriptValue(convMax(psObj, engine));
}

//-- ## setDroidExperience(droid, experience)
//--
//-- Set the amount of experience a droid has. Experience is read using floating point precision.
//--
static QScriptValue js_setDroidExperience(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDroidExperience, context, engine);
}

//-- ## donateObject(object, to)
//--
//-- Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
//-- donation was successful. May return false if this donation would push the receiving player
//-- over unit limits. (3.2+ only)
//--
static QScriptValue js_donateObject(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::donateObject, context, engine);
}

//-- ## donatePower(amount, to)
//--
//-- Donate power to another player. Returns true. (3.2+ only)
//--
static QScriptValue js_donatePower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::donatePower, context, engine);
}

//-- ## safeDest(player, x, y)
//--
//-- Returns true if given player is safe from hostile fire at the given location, to
//-- the best of that player's map knowledge. Does not work in campaign at the moment.
//--
static QScriptValue js_safeDest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::safeDest, context, engine);
}

//-- ## addStructure(structure id, player, x, y)
//--
//-- Create a structure on the given position. Returns the structure on success, null otherwise.
//-- Position uses world coordinates, if you want use position based on Map Tiles, then
//-- use as addStructure(structure id, players, x*128, y*128)
//--
static QScriptValue js_addStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addStructure, context, engine);
}

//-- ## getStructureLimit(structure type[, player])
//--
//-- Returns build limits for a structure.
//--
static QScriptValue js_getStructureLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getStructureLimit, context, engine);
}

//-- ## countStruct(structure type[, player])
//--
//-- Count the number of structures of a given type.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::countStruct, context, engine);
}

//-- ## countDroid([droid type[, player]])
//--
//-- Count the number of droids that a given player has. Droid type must be either
//-- DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::countDroid, context, engine);
}

//-- ## setNoGoArea(x1, y1, x2, y2, player)
//--
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
//--
static QScriptValue js_setNoGoArea(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setNoGoArea, context, engine);
}

//-- ## setScrollLimits(x1, y1, x2, y2)
//--
//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
//--
static QScriptValue js_setScrollLimits(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setScrollLimits, context, engine);
}

//-- ## getScrollLimits()
//--
//-- Get the limits of the scrollable area of the map as an area object. (3.2+ only)
//--
static QScriptValue js_getScrollLimits(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue ret = engine->newObject();
	ret.setProperty("x", scrollMinX, QScriptValue::ReadOnly);
	ret.setProperty("y", scrollMinY, QScriptValue::ReadOnly);
	ret.setProperty("x2", scrollMaxX, QScriptValue::ReadOnly);
	ret.setProperty("y2", scrollMaxY, QScriptValue::ReadOnly);
	ret.setProperty("type", SCRIPT_AREA, QScriptValue::ReadOnly);
	return ret;
}

//-- ## loadLevel(level name)
//--
//-- Load the level with the given name.
//--
static QScriptValue js_loadLevel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::loadLevel, context, engine);
}

//-- ## autoSave()
//--
//-- Perform automatic save
//--
static QScriptValue js_autoSave(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::autoSave, context, engine);
}

//-- ## enumRange(x, y, range[, filter[, seen]])
//--
//-- Returns an array of game objects seen within range of given position that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. Calling this function is much faster than
//-- iterating over all game objects using other enum functions. (3.2+ only)
//--
static QScriptValue js_enumRange(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumRange, context, engine);
}

//-- ## enumArea(<x1, y1, x2, y2 | label>[, filter[, seen]])
//--
//-- Returns an array of game objects seen within the given area that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. The label can either be actual
//-- positions or a label to an AREA. Calling this function is much faster than iterating over all
//-- game objects using other enum functions. (3.2+ only)
//--
static QScriptValue js_enumArea(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(scripting_engine::enumAreaJS, context, engine);
}

//-- ## addBeacon(x, y, target player[, message])
//--
//-- Send a beacon message to target player. Target may also be ```ALLIES```.
//-- Message is currently unused. Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_addBeacon(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addBeacon, context, engine);
}

//-- ## removeBeacon(target player)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_removeBeacon(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::removeBeacon, context, engine);
	if (retVal.isBool() && retVal.toBool())
	{
		jsDebugMessageUpdate();
	}
	return retVal;
}

//-- ## chat(target player, message)
//--
//-- Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_chat(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::chat, context, engine);
}

//-- ## setAlliance(player1, player2, value)
//--
//-- Set alliance status between two players to either true or false. (3.2+ only)
//--
static QScriptValue js_setAlliance(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setAlliance, context, engine);
}

//-- ## sendAllianceRequest(player)
//--
//-- Send an alliance request to a player. (3.3+ only)
//--
static QScriptValue js_sendAllianceRequest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::sendAllianceRequest, context, engine);
}

//-- ## setAssemblyPoint(structure, x, y)
//--
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
//--
static QScriptValue js_setAssemblyPoint(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setAssemblyPoint, context, engine);
}

//-- ## hackNetOff()
//--
//-- Turn off network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOff(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackNetOff, context, engine);
}

//-- ## hackNetOn()
//--
//-- Turn on network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOn(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackNetOn, context, engine);
}

//-- ## getDroidProduction(factory)
//--
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere. (3.2+ only)
//--
static QScriptValue js_getDroidProduction(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getDroidProduction, context, engine);
}

//-- ## getDroidLimit([player[, unit type]])
//--
//-- Return maximum number of droids that this player can produce. This limit is usually
//-- fixed throughout a game and the same for all players. If no arguments are passed,
//-- returns general unit limit for the current player. If a second, unit type argument
//-- is passed, the limit for this unit type is returned, which may be different from
//-- the general unit limit (eg for commanders and construction droids). (3.2+ only)
//--
static QScriptValue js_getDroidLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getDroidLimit, context, engine);
}

//-- ## getExperienceModifier(player)
//--
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_getExperienceModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getExperienceModifier, context, engine);
}

//-- ## setExperienceModifier(player, percent)
//--
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_setExperienceModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setExperienceModifier, context, engine);
}

//-- ## setDroidLimit(player, value[, droid type])
//--
//-- Set the maximum number of droids that this player can produce. If a third
//-- parameter is added, this is the droid type to limit. It can be DROID_ANY
//-- for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
//-- for commanders. (3.2+ only)
//--
static QScriptValue js_setDroidLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDroidLimit, context, engine);
}

//-- ## setCommanderLimit(player, value)
//--
//-- Set the maximum number of commanders that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
static QScriptValue js_setCommanderLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	setMaxCommanders(player, value);
	return QScriptValue();
}

//-- ## setConstructorLimit(player, value)
//--
//-- Set the maximum number of constructors that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
static QScriptValue js_setConstructorLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	setMaxConstructors(player, value);
	return QScriptValue();
}

//-- ## hackAddMessage(message, type, player, immediate)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
static QScriptValue js_hackAddMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::hackAddMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## hackRemoveMessage(message, type, player)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
static QScriptValue js_hackRemoveMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::hackRemoveMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## setSunPosition(x, y, z)
//--
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
//--
static QScriptValue js_setSunPosition(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSunPosition, context, engine);
}

//-- ## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)
//--
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
//--
static QScriptValue js_setSunIntensity(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSunIntensity, context, engine);
}

//-- ## setWeather(weather type)
//--
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
//--
static QScriptValue js_setWeather(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setWeather, context, engine);
}

//-- ## setSky(texture file, wind speed, skybox scale)
//--
//-- Change the skybox. (3.2+ only)
//--
static QScriptValue js_setSky(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSky, context, engine);
}

//-- ## hackDoNotSave(name)
//--
//-- Do not save the given global given by name to savegames. Must be
//-- done again each time game is loaded, since this too is not saved.
//--
static QScriptValue js_hackDoNotSave(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackDoNotSave, context, engine);
}

//-- ## hackMarkTiles([label | x, y[, x2, y2]])
//--
//-- Mark the given tile(s) on the map. Either give a POSITION or AREA label,
//-- or a tile x, y position, or four positions for a square area. If no parameter
//-- is given, all marked tiles are cleared. (3.2+ only)
//--
static QScriptValue js_hackMarkTiles(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackMarkTiles, context, engine);
}

//-- ## cameraSlide(x, y)
//--
//-- Slide the camera over to the given position on the map. (3.2+ only)
//--
static QScriptValue js_cameraSlide(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraSlide, context, engine);
}

//-- ## cameraZoom(z, speed)
//--
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
//--
static QScriptValue js_cameraZoom(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraZoom, context, engine);
}

//-- ## cameraTrack(droid)
//--
//-- Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)
//--
static QScriptValue js_cameraTrack(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraTrack, context, engine);
}

//-- ## setHealth(object, health)
//--
//-- Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.2.3+ only.)
//--
static QScriptValue js_setHealth(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setHealth, context, engine);
}

//-- ## setObjectFlag(object, flag, value)
//--
//-- Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.3+ only.)
//-- Recognized object flags: OBJECT_FLAG_UNSELECTABLE - makes object unavailable for selection from player UI.
//--
static QScriptValue js_setObjectFlag(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setObjectFlag, context, engine);
}

//-- ## addSpotter(x, y, player, range, type, expiry)
//--
//-- Add an invisible viewer at a given position for given player that shows map in given range. ```type```
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)
//--
static QScriptValue js_addSpotter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addSpotter, context, engine);
}

//-- ## removeSpotter(id)
//--
//-- Remove a spotter given its unique ID. (3.2+ only)
//--
static QScriptValue js_removeSpotter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeSpotter, context, engine);
}

//-- ## syncRandom(limit)
//--
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
//--
static QScriptValue js_syncRandom(QScriptContext *context, QScriptEngine * engine)
{
	return wrap_(wzapi::syncRandom, context, engine);
}

//-- ## syncRequest(req_id, x, y[, obj[, obj2]])
//--
//-- Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
//-- Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
//-- take care only to define sync requests that can be validated against cheating. (3.2+ only)
//--
static QScriptValue js_syncRequest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::syncRequest, context, engine);
}

//-- ## replaceTexture(old_filename, new_filename)
//--
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
//--
static QScriptValue js_replaceTexture(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::replaceTexture, context, engine);
}

//-- ## fireWeaponAtLoc(weapon, x, y[, player])
//--
//-- Fires a weapon at the given coordinates (3.3+ only). The player is who owns the projectile.
//-- Please use fireWeaponAtObj() to damage objects as multiplayer and campaign
//-- may have different friendly fire logic for a few weapons (like the lassat).
//--
static QScriptValue js_fireWeaponAtLoc(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::fireWeaponAtLoc, context, engine);
}

//-- ## fireWeaponAtObj(weapon, game object[, player])
//--
//-- Fires a weapon at a game object (3.3+ only). The player is who owns the projectile.
//--
static QScriptValue js_fireWeaponAtObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::fireWeaponAtObj, context, engine);
}

//-- ## changePlayerColour(player, colour)
//--
//-- Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
//--
static QScriptValue js_changePlayerColour(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::changePlayerColour, context, engine);
}

//-- ## getMultiTechLevel()
//--
//-- Returns the current multiplayer tech level. (3.3+ only)
//--
static QScriptValue js_getMultiTechLevel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getMultiTechLevel, context, engine);
}

//-- ## setCampaignNumber(num)
//--
//-- Set the campaign number. (3.3+ only)
//--
static QScriptValue js_setCampaignNumber(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setCampaignNumber, context, engine);
}

//-- ## getMissionType()
//--
//-- Return the current mission type. (3.3+ only)
//--
static QScriptValue js_getMissionType(QScriptContext *context, QScriptEngine *)
{
	return (int)mission.type;
}

//-- ## getRevealStatus()
//--
//-- Return the current fog reveal status. (3.3+ only)
//--
static QScriptValue js_getRevealStatus(QScriptContext *context, QScriptEngine *)
{
	return QScriptValue(getRevealStatus());
}

//-- ## setRevealStatus(bool)
//--
//-- Set the fog reveal status. (3.3+ only)
static QScriptValue js_setRevealStatus(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setRevealStatus, context, engine);
}


QScriptValue js_stats(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue callee = context->callee();
	int type = callee.property("type").toInt32();
	int player = callee.property("player").toInt32();
	unsigned index = callee.property("index").toUInt32();
	QString name = callee.property("name").toString();
	qtscript_execution_context execution_context(context, engine);
	if (context->argumentCount() == 1) // setter
	{
		wzapi::setUpgradeStats(execution_context, player, name.toStdString(), type, index, context->argument(0).toVariant());
	}
	// Now read value and return it
	return mapJsonToQScriptValue(engine, wzapi::getUpgradeStats(execution_context, player, name.toStdString(), type, index), QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

static void setStatsFunc(QScriptValue &base, QScriptEngine *engine, const QString& name, int player, int type, int index)
{
	QScriptValue v = engine->newFunction(js_stats);
	base.setProperty(name, v, QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
	v.setProperty("player", player, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("type", type, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("index", index, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("name", name, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

nlohmann::json scripting_engine::constructDerrickPositions()
{
	// Static map knowledge about start positions
	//== * ```derrickPositions``` An array of derrick starting positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a derrick.
	nlohmann::json derrickPositions = nlohmann::json::array(); //engine->newArray(derricks.size());
	for (int i = 0; i < derricks.size(); i++)
	{
		nlohmann::json vector = nlohmann::json::object();
		vector["x"] = map_coord(derricks[i].x);
		vector["y"] = map_coord(derricks[i].y);
		vector["type"] = SCRIPT_POSITION;
		derrickPositions.push_back(vector);
	}
	return derrickPositions;
}

nlohmann::json scripting_engine::constructStartPositions()
{
	// Static map knowledge about start positions
	//== * ```startPositions``` An array of player start positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a player start position.
	nlohmann::json startPositions = nlohmann::json::array(); //engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		nlohmann::json vector = nlohmann::json::object();
		vector["x"] = map_coord(positions[i].x);
		vector["y"] = map_coord(positions[i].y);
		vector["type"] = SCRIPT_POSITION;
		startPositions.push_back(vector);
	}
	return startPositions;
}

QScriptValue constructUpgradesQScriptValue(QScriptEngine *engine)
{
	auto upgradesObj = wzapi::getUpgradesObject();

	QScriptValue upgrades = engine->newArray(MAX_PLAYERS);
	for (auto& playerUpgrades : upgradesObj)
	{
		QScriptValue node = engine->newObject();

		for (const auto& gameEntityClass : playerUpgrades)
		{
			const std::string& gameEntityClassName = gameEntityClass.first;
			QScriptValue entityBase = engine->newObject();
			for (const auto& gameEntity : gameEntityClass.second)
			{
				const std::string& gameEntityName = gameEntity.first;
				const auto& gameEntityRules = gameEntity.second;
				QScriptValue v = engine->newObject();
				for (const auto& property : gameEntityRules)
				{
					setStatsFunc(v, engine, QString::fromUtf8(property.first.c_str()), gameEntityRules.getPlayer(), property.second, gameEntityRules.getIndex());
				}
				entityBase.setProperty(QString::fromUtf8(gameEntityName.c_str()), v, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			node.setProperty(QString::fromUtf8(gameEntityClassName.c_str()), entityBase, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}

		// Finally
		ASSERT(playerUpgrades.getPlayer() >= 0 && playerUpgrades.getPlayer() < MAX_PLAYERS, "Invalid player index");
		upgrades.setProperty(playerUpgrades.getPlayer(), node, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}

	return upgrades;
}

bool quickjs_scripting_instance::registerFunctions(const std::string& scriptName)
{
	debug(LOG_WZ, "Loading functions for engine %p, script %s", static_cast<void *>(engine), scriptName.toUtf8().constData());

	// Register 'Stats' object. It is a read-only representation of basic game component states.
	nlohmann::json stats = wzapi::constructStatsObject();
	engine->globalObject().setProperty("Stats", mapJsonToQScriptValue(engine, stats, QScriptValue::ReadOnly | QScriptValue::Undeletable), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	//== * ```Upgrades``` A special array containing per-player rules information for game entity types,
	//== which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
	//== array contains a subset of the sparse array of rules information in the ```Stats``` global.
	//== These values are defined:
	QScriptValue upgrades = constructUpgradesQScriptValue(engine);
	engine->globalObject().setProperty("Upgrades", upgrades, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate)); // WZAPI
	engine->globalObject().setProperty("dump", engine->newFunction(js_dump));
	engine->globalObject().setProperty("syncRandom", engine->newFunction(js_syncRandom)); // WZAPI
	engine->globalObject().setProperty("label", engine->newFunction(js_getObject)); // deprecated // scripting_engine
	engine->globalObject().setProperty("getObject", engine->newFunction(js_getObject)); // scripting_engine
	engine->globalObject().setProperty("addLabel", engine->newFunction(js_addLabel)); // scripting_engine
	engine->globalObject().setProperty("removeLabel", engine->newFunction(js_removeLabel)); // scripting_engine
	engine->globalObject().setProperty("getLabel", engine->newFunction(js_getLabel)); // scripting_engine
	engine->globalObject().setProperty("enumLabels", engine->newFunction(js_enumLabels)); // scripting_engine
	engine->globalObject().setProperty("enumGateways", engine->newFunction(js_enumGateways));
	engine->globalObject().setProperty("enumTemplates", engine->newFunction(js_enumTemplates));
	engine->globalObject().setProperty("makeTemplate", engine->newFunction(js_makeTemplate)); // WZAPI
	engine->globalObject().setProperty("setAlliance", engine->newFunction(js_setAlliance)); // WZAPI
	engine->globalObject().setProperty("sendAllianceRequest", engine->newFunction(js_sendAllianceRequest)); // WZAPI
	engine->globalObject().setProperty("setAssemblyPoint", engine->newFunction(js_setAssemblyPoint)); // WZAPI
	engine->globalObject().setProperty("setSunPosition", engine->newFunction(js_setSunPosition)); // WZAPI
	engine->globalObject().setProperty("setSunIntensity", engine->newFunction(js_setSunIntensity)); // WZAPI
	engine->globalObject().setProperty("setWeather", engine->newFunction(js_setWeather)); // WZAPI
	engine->globalObject().setProperty("setSky", engine->newFunction(js_setSky)); // WZAPI
	engine->globalObject().setProperty("cameraSlide", engine->newFunction(js_cameraSlide)); // WZAPI
	engine->globalObject().setProperty("cameraTrack", engine->newFunction(js_cameraTrack)); // WZAPI
	engine->globalObject().setProperty("cameraZoom", engine->newFunction(js_cameraZoom)); // WZAPI
	engine->globalObject().setProperty("resetArea", engine->newFunction(js_resetLabel)); // deprecated // scripting_engine
	engine->globalObject().setProperty("resetLabel", engine->newFunction(js_resetLabel)); // scripting_engine
	engine->globalObject().setProperty("addSpotter", engine->newFunction(js_addSpotter)); // WZAPI
	engine->globalObject().setProperty("removeSpotter", engine->newFunction(js_removeSpotter)); // WZAPI
	engine->globalObject().setProperty("syncRequest", engine->newFunction(js_syncRequest)); // WZAPI
	engine->globalObject().setProperty("replaceTexture", engine->newFunction(js_replaceTexture)); // WZAPI
	engine->globalObject().setProperty("changePlayerColour", engine->newFunction(js_changePlayerColour)); // WZAPI
	engine->globalObject().setProperty("setHealth", engine->newFunction(js_setHealth)); // WZAPI
	engine->globalObject().setProperty("useSafetyTransport", engine->newFunction(js_useSafetyTransport)); // WZAPI
	engine->globalObject().setProperty("restoreLimboMissionData", engine->newFunction(js_restoreLimboMissionData)); // WZAPI
	engine->globalObject().setProperty("getMultiTechLevel", engine->newFunction(js_getMultiTechLevel)); // WZAPI
	engine->globalObject().setProperty("setCampaignNumber", engine->newFunction(js_setCampaignNumber)); // WZAPI
	engine->globalObject().setProperty("getMissionType", engine->newFunction(js_getMissionType));
	engine->globalObject().setProperty("getRevealStatus", engine->newFunction(js_getRevealStatus));
	engine->globalObject().setProperty("setRevealStatus", engine->newFunction(js_setRevealStatus)); // WZAPI
	engine->globalObject().setProperty("autoSave", engine->newFunction(js_autoSave)); // WZAPI

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff)); // WZAPI
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn)); // WZAPI
	engine->globalObject().setProperty("hackAddMessage", engine->newFunction(js_hackAddMessage)); // WZAPI
	engine->globalObject().setProperty("hackRemoveMessage", engine->newFunction(js_hackRemoveMessage)); // WZAPI
	engine->globalObject().setProperty("objFromId", engine->newFunction(js_objFromId));
	engine->globalObject().setProperty("hackGetObj", engine->newFunction(js_hackGetObj)); // WZAPI
	engine->globalObject().setProperty("hackChangeMe", engine->newFunction(js_hackChangeMe)); // WZAPI
	engine->globalObject().setProperty("hackAssert", engine->newFunction(js_hackAssert)); // WZAPI
	engine->globalObject().setProperty("hackMarkTiles", engine->newFunction(js_hackMarkTiles)); // WZAPI
	engine->globalObject().setProperty("receiveAllEvents", engine->newFunction(js_receiveAllEvents)); // WZAPI
	engine->globalObject().setProperty("hackDoNotSave", engine->newFunction(js_hackDoNotSave)); // WZAPI
	engine->globalObject().setProperty("hackPlayIngameAudio", engine->newFunction(js_hackPlayIngameAudio)); // WZAPI
	engine->globalObject().setProperty("hackStopIngameAudio", engine->newFunction(js_hackStopIngameAudio)); // WZAPI

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console)); // WZAPI
	engine->globalObject().setProperty("clearConsole", engine->newFunction(js_clearConsole)); // WZAPI
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle)); // WZAPI
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct)); // WZAPI
	engine->globalObject().setProperty("enumStructOffWorld", engine->newFunction(js_enumStructOffWorld)); // WZAPI
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid)); // WZAPI
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup)); // scripting_engine
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature)); // WZAPI
	engine->globalObject().setProperty("enumBlips", engine->newFunction(js_enumBlips)); // WZAPI
	engine->globalObject().setProperty("enumSelected", engine->newFunction(js_enumSelected)); // WZAPI
	engine->globalObject().setProperty("enumResearch", engine->newFunction(js_enumResearch)); // WZAPI
	engine->globalObject().setProperty("enumRange", engine->newFunction(js_enumRange)); // WZAPI
	engine->globalObject().setProperty("enumArea", engine->newFunction(js_enumArea)); // scripting_engine
	engine->globalObject().setProperty("getResearch", engine->newFunction(js_getResearch)); // WZAPI
	engine->globalObject().setProperty("pursueResearch", engine->newFunction(js_pursueResearch)); // WZAPI
	engine->globalObject().setProperty("findResearch", engine->newFunction(js_findResearch)); // WZAPI
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints)); // WZAPI
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup)); // scripting_engine
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea)); // scripting_engine
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid)); // scripting_engine
	engine->globalObject().setProperty("groupAdd", engine->newFunction(js_groupAdd)); // scripting_engine
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize)); // scripting_engine
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc)); // WZAPI
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower)); // WZAPI
	engine->globalObject().setProperty("queuedPower", engine->newFunction(js_queuedPower)); // WZAPI
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable)); // WZAPI
	engine->globalObject().setProperty("pickStructLocation", engine->newFunction(js_pickStructLocation)); // WZAPI
	engine->globalObject().setProperty("droidCanReach", engine->newFunction(js_droidCanReach)); // WZAPI
	engine->globalObject().setProperty("propulsionCanReach", engine->newFunction(js_propulsionCanReach)); // WZAPI
	engine->globalObject().setProperty("terrainType", engine->newFunction(js_terrainType)); // WZAPI
	engine->globalObject().setProperty("orderDroidBuild", engine->newFunction(js_orderDroidBuild)); // WZAPI
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj)); // WZAPI
	engine->globalObject().setProperty("orderDroid", engine->newFunction(js_orderDroid)); // WZAPI
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid)); // WZAPI
	engine->globalObject().setProperty("addDroid", engine->newFunction(js_addDroid)); // WZAPI
	engine->globalObject().setProperty("addDroidToTransporter", engine->newFunction(js_addDroidToTransporter)); // WZAPI
	engine->globalObject().setProperty("addFeature", engine->newFunction(js_addFeature)); // WZAPI
	engine->globalObject().setProperty("componentAvailable", engine->newFunction(js_componentAvailable)); // WZAPI
	engine->globalObject().setProperty("isVTOL", engine->newFunction(js_isVTOL)); // WZAPI
	engine->globalObject().setProperty("safeDest", engine->newFunction(js_safeDest)); // WZAPI
	engine->globalObject().setProperty("activateStructure", engine->newFunction(js_activateStructure)); // WZAPI
	engine->globalObject().setProperty("chat", engine->newFunction(js_chat)); // WZAPI
	engine->globalObject().setProperty("addBeacon", engine->newFunction(js_addBeacon)); // WZAPI
	engine->globalObject().setProperty("removeBeacon", engine->newFunction(js_removeBeacon)); // WZAPI
	engine->globalObject().setProperty("getDroidProduction", engine->newFunction(js_getDroidProduction)); // WZAPI
	engine->globalObject().setProperty("getDroidLimit", engine->newFunction(js_getDroidLimit)); // WZAPI
	engine->globalObject().setProperty("getExperienceModifier", engine->newFunction(js_getExperienceModifier)); // WZAPI
	engine->globalObject().setProperty("setDroidLimit", engine->newFunction(js_setDroidLimit)); // WZAPI
	engine->globalObject().setProperty("setCommanderLimit", engine->newFunction(js_setCommanderLimit)); // deprecated!!
	engine->globalObject().setProperty("setConstructorLimit", engine->newFunction(js_setConstructorLimit)); // deprecated!!
	engine->globalObject().setProperty("setExperienceModifier", engine->newFunction(js_setExperienceModifier)); // WZAPI
	engine->globalObject().setProperty("getWeaponInfo", engine->newFunction(js_getWeaponInfo)); // deprecated!!
	engine->globalObject().setProperty("enumCargo", engine->newFunction(js_enumCargo)); // WZAPI

	// Functions that operate on the current player only
	engine->globalObject().setProperty("centreView", engine->newFunction(js_centreView)); // WZAPI
	engine->globalObject().setProperty("playSound", engine->newFunction(js_playSound)); // WZAPI
	engine->globalObject().setProperty("gameOverMessage", engine->newFunction(js_gameOverMessage)); // WZAPI

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	engine->globalObject().setProperty("setStructureLimits", engine->newFunction(js_setStructureLimits)); // WZAPI
	engine->globalObject().setProperty("applyLimitSet", engine->newFunction(js_applyLimitSet)); // WZAPI
	engine->globalObject().setProperty("setMissionTime", engine->newFunction(js_setMissionTime)); // WZAPI
	engine->globalObject().setProperty("getMissionTime", engine->newFunction(js_getMissionTime)); // WZAPI
	engine->globalObject().setProperty("setReinforcementTime", engine->newFunction(js_setReinforcementTime)); // WZAPI
	engine->globalObject().setProperty("completeResearch", engine->newFunction(js_completeResearch)); // WZAPI
	engine->globalObject().setProperty("completeAllResearch", engine->newFunction(js_completeAllResearch)); // WZAPI
	engine->globalObject().setProperty("enableResearch", engine->newFunction(js_enableResearch)); // WZAPI
	engine->globalObject().setProperty("setPower", engine->newFunction(js_setPower)); // WZAPI
	engine->globalObject().setProperty("setPowerModifier", engine->newFunction(js_setPowerModifier)); // WZAPI
	engine->globalObject().setProperty("setPowerStorageMaximum", engine->newFunction(js_setPowerStorageMaximum)); // WZAPI
	engine->globalObject().setProperty("extraPowerTime", engine->newFunction(js_extraPowerTime)); // WZAPI
	engine->globalObject().setProperty("setTutorialMode", engine->newFunction(js_setTutorialMode)); // WZAPI
	engine->globalObject().setProperty("setDesign", engine->newFunction(js_setDesign)); // WZAPI
	engine->globalObject().setProperty("enableTemplate", engine->newFunction(js_enableTemplate)); // WZAPI
	engine->globalObject().setProperty("removeTemplate", engine->newFunction(js_removeTemplate)); // WZAPI
	engine->globalObject().setProperty("setMiniMap", engine->newFunction(js_setMiniMap)); // WZAPI
	engine->globalObject().setProperty("setReticuleButton", engine->newFunction(js_setReticuleButton)); // WZAPI
	engine->globalObject().setProperty("setReticuleFlash", engine->newFunction(js_setReticuleFlash)); // WZAPI
	engine->globalObject().setProperty("showReticuleWidget", engine->newFunction(js_showReticuleWidget)); // WZAPI
	engine->globalObject().setProperty("showInterface", engine->newFunction(js_showInterface)); // WZAPI
	engine->globalObject().setProperty("hideInterface", engine->newFunction(js_hideInterface)); // WZAPI
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_removeReticuleButton)); // deprecated!!
	engine->globalObject().setProperty("removeReticuleButton", engine->newFunction(js_removeReticuleButton)); // deprecated!!
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure)); // WZAPI
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable)); // WZAPI
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent)); // WZAPI
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween)); // WZAPI
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct)); // WZAPI // deprecated!!
	engine->globalObject().setProperty("removeObject", engine->newFunction(js_removeObject)); // WZAPI
	engine->globalObject().setProperty("setScrollParams", engine->newFunction(js_setScrollLimits)); // deprecated!!
	engine->globalObject().setProperty("setScrollLimits", engine->newFunction(js_setScrollLimits)); // WZAPI
	engine->globalObject().setProperty("getScrollLimits", engine->newFunction(js_getScrollLimits));
	engine->globalObject().setProperty("addStructure", engine->newFunction(js_addStructure)); // WZAPI
	engine->globalObject().setProperty("getStructureLimit", engine->newFunction(js_getStructureLimit)); // WZAPI
	engine->globalObject().setProperty("countStruct", engine->newFunction(js_countStruct)); // WZAPI
	engine->globalObject().setProperty("countDroid", engine->newFunction(js_countDroid)); // WZAPI
	engine->globalObject().setProperty("loadLevel", engine->newFunction(js_loadLevel)); // WZAPI
	engine->globalObject().setProperty("setDroidExperience", engine->newFunction(js_setDroidExperience)); // WZAPI
	engine->globalObject().setProperty("donateObject", engine->newFunction(js_donateObject)); // WZAPI
	engine->globalObject().setProperty("donatePower", engine->newFunction(js_donatePower)); // WZAPI
	engine->globalObject().setProperty("setNoGoArea", engine->newFunction(js_setNoGoArea)); // WZAPI
	engine->globalObject().setProperty("startTransporterEntry", engine->newFunction(js_startTransporterEntry)); // WZAPI
	engine->globalObject().setProperty("setTransporterExit", engine->newFunction(js_setTransporterExit)); // WZAPI
	engine->globalObject().setProperty("setObjectFlag", engine->newFunction(js_setObjectFlag)); // WZAPI
	engine->globalObject().setProperty("fireWeaponAtLoc", engine->newFunction(js_fireWeaponAtLoc)); // WZAPI
	engine->globalObject().setProperty("fireWeaponAtObj", engine->newFunction(js_fireWeaponAtObj)); // WZAPI

	// Set some useful constants
	setSpecifiedGlobalVariables(wzapi::getUsefulConstants());

	/// Place to store group sizes
	//== * ```groupSizes``` A sparse array of group sizes. If a group has never been used, the entry in this array will
	//== be undefined.
	engine->globalObject().setProperty("groupSizes", engine->newObject());

	// Static knowledge about players
	nlohmann::json playerData = wzapi::constructStaticPlayerData();
	engine->globalObject().setProperty("playerData", mapJsonToQScriptValue(engine, playerData, QScriptValue::ReadOnly | QScriptValue::Undeletable), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Static map knowledge about start positions
	nlohmann::json startPositions = scripting_engine::instance().constructStartPositions();
	nlohmann::json derrickPositions = scripting_engine::instance().constructDerrickPositions();
	engine->globalObject().setProperty("derrickPositions", mapJsonToQScriptValue(engine, derrickPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("startPositions", mapJsonToQScriptValue(engine, startPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Clear previous log file
	PHYSFS_delete(QString("logs/" + scriptName + ".log").toUtf8().constData());

	return true;
}

// Enable JSON support for custom types

// QVariant
void to_json(nlohmann::json& j, const JSContextValue& v) {
	// IMPORTANT: This largely follows the Qt documentation on QJsonValue::fromVariant
	// See: http://doc.qt.io/qt-5/qjsonvalue.html#fromVariant
	//
	// The main change is that numeric types are independently handled (instead of
	// converting everything to `double`), because nlohmann::json handles them a bit
	// differently.

	// Note: Older versions of Qt 5.x (5.6?) do not define QMetaType::Nullptr,
	//		 so check value.isNull() instead.
	if (JS_IsNull(v.value))
	{
		j = nlohmann::json(); // null value
		return;
	}

	if (JS_IsArray(v.ctx, v.value))
	{
		j = nlohmann::json::array();
		uint64_t length = 0;
		if (QuickJS_GetArrayLength(v.ctx, v.value, length))
		{
			for (uint64_t k = 0; k < length; k++) // TODO: uint64_t isn't correct here, as we call GetUint32...
			{
				JSValue jsVal = JS_GetPropertyUint32(v.ctx, v.value, k);
				nlohmann::json jsonValue;
				to_json(jsonValue, JSContextValue{v.ctx, jsVal});
				j.push_back(jsonValue);
				JS_FreeValue(ctx, resVal);
			}
		}
		return;
	}
	if (JS_IsObject(v.ctx, v.value))
	{
		j = nlohmann::json::object();
		QuickJS_EnumerateObjectProperties(v.ctx, v.value, [v, result](const char *key, JSAtom &atom) {
			JSValue jsVal = JS_GetProperty(v.ctx, v.value, atom);
			std::string nameStr = key;
			nlohmann::json jsonValue;
			to_json(jsonValue, JSContextValue{v.ctx, jsVal});
			j[nameStr] = jsonValue;
			JS_FreeValue(ctx, jsVal);
		});
	}

	int tag = JS_VALUE_GET_NORM_TAG(v.value);
	switch (tag)
	{
		case JS_TAG_BOOL:
			j = JS_ToBool(v.ctx, v.value);
			break;
		case JS_TAG_INT;
		{
			int32_t intVal;
			if (JS_ToInt32(v.ctx, &intVal, v.value))
			{
				// Failed
			}
			j = intVal;
			break;
		}
		case JS_TAG_FLOAT64:
		{
			double dblVal;
			if (JS_ToFloat64(v.ctx, &dblVal, v.value))
			{
				// Failed
			}
			j = dblVal;
			break;
		}
		case JS_TAG_UNDEFINED:
			j = nlohmann::json(); // null value // ???
			break;
		case JS_TAG_STRING:
		{
			const char* pStr = JS_ToCString(v.ctx, v.value);
			j = json((pStr) ? pStr : "");
			JS_FreeCString(ctx, pStr);
			break;
		}
		default:
			// In every other case, a conversion to a string will be attempted
	}
//
//	switch (value.userType())
//	{
//#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
//		case QMetaType::Nullptr:
//			j = nlohmann::json(); // null value
//			break;
//#endif
//		case QMetaType::Bool:
//			j = value.toBool();
//			break;
//		case QMetaType::Int:
//			j = value.toInt();
//			break;
//		case QMetaType::UInt:
//			j = value.toUInt();
//			break;
//		case QMetaType::LongLong:
//			j = value.toLongLong();
//			break;
//		case QMetaType::ULongLong:
//			j = value.toULongLong();
//			break;
//		case QMetaType::Float:
//		case QVariant::Double:
//			j = value.toDouble();
//			break;
//		case QMetaType::QString:
//		{
//			QString qstring = value.toString();
//			j = json(qstring.toUtf8().constData());
//			break;
//		}
//		case QMetaType::QStringList:
//		case QMetaType::QVariantList:
//		{
//			// an array
//			j = nlohmann::json::array();
//			QList<QVariant> list = value.toList();
//			for (QVariant& list_variant : list)
//			{
//				nlohmann::json list_variant_json_value;
//				to_json(list_variant_json_value, list_variant);
//				j.push_back(list_variant_json_value);
//			}
//			break;
//		}
//		case QMetaType::QVariantMap:
//		{
//			// an object
//			j = nlohmann::json::object();
//			QMap<QString, QVariant> map = value.toMap();
//			for (QMap<QString, QVariant>::const_iterator i = map.constBegin(); i != map.constEnd(); ++i)
//			{
//				j[i.key().toUtf8().constData()] = i.value();
//			}
//			break;
//		}
//		case QMetaType::QVariantHash:
//		{
//			// an object
//			j = nlohmann::json::object();
//			QHash<QString, QVariant> hash = value.toHash();
//			for (QHash<QString, QVariant>::const_iterator i = hash.constBegin(); i != hash.constEnd(); ++i)
//			{
//				j[i.key().toUtf8().constData()] = i.value();
//			}
//			break;
//		}
//		default:
//			// For all other QVariant types a conversion to a QString will be attempted.
//			QString qstring = value.toString();
//			// If the returned string is empty, a Null QJsonValue will be stored, otherwise a String value using the returned QString.
//			if (qstring.isEmpty())
//			{
//				j = nlohmann::json(); // null value
//			}
//			else
//			{
//				j = std::string(qstring.toUtf8().constData());
//			}
//	}
}

