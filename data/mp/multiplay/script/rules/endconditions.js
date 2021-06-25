// registrate events conditions_eventGameInit, conditions_eventDroidBuilt, conditions_eventStructureBuil, conditions_eventResearched, conditions_eventAttacked
namespace("conditions_");

const STATE_contender = "contender";
const STATE_winner = "winner";
const STATE_loser = "loser";
const STATE_spectator = "spectator";
const STRUCTS = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY]; // structures in which you can continue to play


// The time that the player's inactivity is allowed. Actions are considered
// - unit building
// - completion of the research
// - construction of base structures (factories, power plants, laboratories, modules and oil rigs)
// - dealing damage
const IDLETIME = 5 * 60 * 1000;
const BASESTRUCTS = [FACTORY, CYBORG_FACTORY, VTOL_FACTORY, HQ, RESOURCE_EXTRACTOR, POWER_GEN, RESEARCH_LAB];
const ENABLE_activity = (challenge != true && isMultiplayer === true); //The prohibition on passive play can interfere when playing against bots. There is no reason to end a fight earlier in PVE.
//const ENABLE_activity = true; //debug

var teams; // array class instance Team
var playersTeam; // array class instancePlayer

class Player
{
	constructor(playNum)
	{
		this.playNum = playNum;
	}

	hasFactory()
	{
		for (let i = 0; i < STRUCTS.length; ++i)
		{
			const onMapStructss = enumStruct(this.playNum, STRUCTS[i]);
			for (let j = 0; j < onMapStructss.length; ++j)
			{
				if (onMapStructss[j].status === BUILT)
				{
					return true;
				}
			}
		}
		return false;
	}

	hasDroid()
	{
		if (countDroid(DROID_ANY, this.playNum) > 0)
		{
			return true;
		}
		return false;
	}

	hasOnlyConstructor()
	{
		if (countDroid(DROID_ANY, this.playNum) - countDroid(DROID_CONSTRUCT, this.playNum) === 0)
		{
			return true;
		}
		return false;
	}

	canReachOil()
	{
		if (enumStruct(this.playNum, RESOURCE_EXTRACTOR).length != 0)
		{
			return true;
		}

		let oils = enumFeature(ALL_PLAYERS).filter(function(e)
		{
			return e.stattype === OIL_RESOURCE;
		});
		for (let playnum = 0; playnum < maxPlayers; playnum++)
		{
			oils = oils.concat(enumStruct(playnum, "A0ResourceExtractor"));
		}

		const trucks = enumDroid(this.playNum, DROID_CONSTRUCT);
		for (let i = 0, len = trucks.length; i < len; ++i)
		{
			const truck = trucks[i];
			for (let j = 0, len2 = oils.length; j < len2; ++j)
			{
				const oil = oils[j];
				if (droidCanReach(truck, oil.x, oil.y))
				{
					return true;
				}
			}
		}
		return false;
	}

	finalizeGame(state)
	{
		if (!isSpectator(this.playNum) && playerData[this.playNum].isHuman)
		{
			transformPlayerToSpectator(this.playNum);
		}
		if (state === STATE_loser && this.playNum != selectedPlayer)
		{
			gameOverMessage(false);
		}
		if (state === STATE_winner && this.playNum != selectedPlayer)
		{
			gameOverMessage(true);
		}
	}
}


class Team
{
	constructor(playerPlayNums)
	{
		this.players = playerPlayNums.map(function(playNum)
		{
			return new Player(playNum);
		}); // array class instance  Player
		this.lastActivity = gameTime;
		playerPlayNums.forEach(
			(playerNum) =>
			{
				playersTeam[playerNum]= this;
			}
		);
	}

	activeGame()
	{
		if (this.lastActivity + IDLETIME >= gameTime)
		{
			return true;
		}
		return false;
	}

	hasFactory()
	{
		return this.players.some(
			(player) => {return player.hasFactory();}
		);
	}

	hasDroid()
	{
		return this.players.some(
			(player) => {return player.hasDroid();}
		);
	}

	hasOnlyConstructor()
	{
		return this.players.some(
			(player) => {return player.hasOnlyConstructor();}
		);
	}

	canReachOil()
	{
		return this.players.some(
			(player) => {return player.canReachOil();}
		);
	}

	hasSpectator()
	{
		return this.players.some(
			(player) => {return isSpectator(player.playNum);}
		);
	}

	canPlay() // TODO skip check if no new events.
	{
		if (this.hasSpectator())
		{
			return false;
		}
		if (!this.activeGame() && ENABLE_activity)
		{
			return false;
		}
		if (!this.hasFactory() && !this.hasDroid())
		{
			return false;
		}
		if (!this.hasFactory() && this.hasOnlyConstructor() && !this.canReachOil())
		{
			return false;
		}
		return true;
	}

	setState(state)
	{
		this.state = state;
		if (state ===  STATE_winner || state === STATE_loser ||  state === STATE_spectator)
		{
			this.players.forEach(
				(player) =>
				{
					player.finalizeGame(this.state);
				}
			);
		}
	}

	isContender()
	{
		return this.state === STATE_contender;
	}
}

function checkEndConditions()
{
	teams.filter((team) =>
	{
		return (team.isContender() && !team.canPlay());
	}).forEach((team) =>
	{
		team.setState(STATE_loser);
	});
	const numTeamContender = teams.filter((team) =>
	{
		return team.isContender();
	}).length;
	if (numTeamContender === 1) // game end
	{
		teams.find((team) =>
		{
			return team.isContender();
		}).setState(STATE_winner);
	}
}

//	FIXME allianceExistsBetween() dont correct if leave player in ALLIANCES_UNSHARED, ALLIANCES_TEAMS mode
//	and  team is garbage in NO_ALLIANCES, ALLIANCES mode
function inOneTeam(playnum, splaynum)
{

	if (
		(alliancesType === ALLIANCES_UNSHARED || alliancesType === ALLIANCES_TEAMS) &&
    playerData[playnum].team === playerData[splaynum].team
	)
	{
		return true;
	}
	else if (alliancesType === NO_ALLIANCES && playnum === splaynum)
	{
		return true;
	}
	//Victory in alliance mode is also personal.
	//Alliances do not affect victory.
	//allianceExistsBetween() is not used.
	else if (alliancesType === ALLIANCES && playnum === splaynum)
	{
		return true;
	}
	return false;
}

function createTeams()
{
	teams = [];
	playersTeam = new Array(maxPlayers);
	const inTeamPlayNums = new Array(maxPlayers).fill(false);
	for (let playNum = 0; playNum < maxPlayers; playNum++)
	{
		if (inTeamPlayNums[playNum] === true)
		{
			continue;
		}
		inTeamPlayNums[playNum] = true;
		const members =[playNum];
		for (let splayNum = 0; splayNum < maxPlayers; splayNum++)
		{
			if ( inTeamPlayNums[splayNum] === false && inOneTeam(playNum, splayNum) === true)
			{
				members.push(splayNum);
				inTeamPlayNums[splayNum] = true;
			}
		}
		const team = new Team(members);
		if (team.canPlay())
		{
			teams.push(team);
			team.setState(STATE_contender);
		}
		else
		{
			team.setState(STATE_spectator);
		}
	}

}

/////////////////////////////////////
//First start and loading the save.//
/////////////////////////////////////

function conditions_eventGameInit()
{
	createTeams();
	//find old type spectators
	if  (ENABLE_activity)
	{
		setTimer("activityAlert", 10*1000);
	}
	setTimer("checkEndConditions", 3000);
}

function conditions_eventGameLoaded()
{
	createTeams();
}

///////////////////////////////////////////////////////////////////////////
//Logging for active actions and displaying warnings during passive play.//
///////////////////////////////////////////////////////////////////////////

function activityAlert()
{
	if (playersTeam[selectedPlayer].state != STATE_contender)
	{
		setMissionTime(-1);
		removeTimer("activityAlert");
		return;
	}
	if (playersTeam[selectedPlayer].lastActivity + IDLETIME / 2 < gameTime)
	{
		console(
			_("Playing passively will lead to defeat. Actions that are considered: - unit building - research completion - construction of base structures (factories, power plants, laboratories, modules and oil derricks) - dealing damage")
		);
		if (getMissionTime() > IDLETIME)
		{
			setMissionTime(
				(playersTeam[selectedPlayer].lastActivity + IDLETIME - gameTime) / 1000);
		}
	}
	if (playersTeam[selectedPlayer].lastActivity + IDLETIME / 2 > gameTime)
	{
		setMissionTime(-1); // remove timer widget
	}
}
function conditions_eventDroidBuilt(droid)
{
	if (droid.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[droid.player])
	{
		playersTeam[droid.player].lastActivity = gameTime;
	}
	else
	{
		debug ("Player", droid.player, "has no team eventDroidBuilt");
	}

}
function conditions_eventStructureBuilt(structure)
{
	if (structure.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (BASESTRUCTS.includes(structure.stattype) === true && playersTeam[structure.player])
	{
		playersTeam[structure.player].lastActivity = gameTime;
	}
	if (!playersTeam[structure.player])
	{
		debug ("Player", structure.player, "has no team eventStructureBuilt");
	}
}
function conditions_eventResearched(research, structure, player)
{
	if (player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[player])
	{
		playersTeam[player].lastActivity = gameTime;
	}
	else
	{
		debug ("Player", player, "has no team eventResearched");
	}
}
function conditions_eventAttacked(victim, attacker)
{
	if (attacker.player === scavengerPlayer || !ENABLE_activity)
	{
		return;
	}
	if (playersTeam[attacker.player])
	{
		playersTeam[attacker.player].lastActivity = gameTime;
	}
	else
	{
		debug ("Player", attacker.player, "has no team eventAttacked");
	}
}
