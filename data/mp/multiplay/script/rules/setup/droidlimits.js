const BASE_UNIT_LIMIT = 150;
const BASE_COMMANDER_LIMIT = 10;
const BASE_CONSTRUCTOR_LIMIT = 15;

function commanderLimitForUnitLimit(unitLimit)
{
	return Math.max(BASE_COMMANDER_LIMIT, Math.floor(unitLimit * BASE_COMMANDER_LIMIT / BASE_UNIT_LIMIT));
}

function constructorLimitForUnitLimit(unitLimit)
{
	return BASE_CONSTRUCTOR_LIMIT + Math.floor(Math.max(0, unitLimit - BASE_UNIT_LIMIT) / 30);
}

function droidLimit(player)	// inside hackNetOff()
{
	setDroidLimit(player, unitLimitSetting, DROID_ANY);
	setDroidLimit(player, commanderLimitForUnitLimit(unitLimitSetting), DROID_COMMAND);
	setDroidLimit(player, constructorLimitForUnitLimit(unitLimitSetting), DROID_CONSTRUCT);
}
