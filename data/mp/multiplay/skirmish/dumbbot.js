//#region vscode 1.88.0+
//debug(JSON.stringify(includeJSON("nexus.json")))
const DEBUG=("25a"=="".concat("@","{","V","E","R","S","I","O","N","}"))

const home=startPositions[me]
const center={x:mapWidth/2, y:mapHeight/2}
const players=Array(maxPlayers).fill(0).map((i,index)=>index)
const oilCount=derrickPositions.filter(i=>propulsionCanReach("wheeled01",home.x,home.y,i.x,i.y)).length
var oilRatio=oilCount/playerData.length
var structurePend=false
/**  */
var attackedByArtillery=0
//{"ANTI PERSONNEL":0, "ANTI AIRCRAFT":0, "ARTILLERY ROUND":0, "ANTI TANK":0, "FLAMER":0, "BUNKER BUSTER":0}

/** @type {_pos[]} */
const attackPath=[] //??? Avoid failure at Sk-FishNets

const FLATMAP=(derrickPositions.length/players.filter(i=>!isSpectator(i)).length)>30

const SCHEMA_MACGUN=0
const SCHEMA_CANNON=1
const SCHEMA_ROCKET=2
const SCHEMA_MORTAR=3
const SCHEMA_FLAMER=4
const SCHEMA_HPVCAN=5
function chatDebug(...objs)
{
    var s=objs.map(i=>JSON.stringify(i)).join(" ")
    chat(ALL_PLAYERS,s)
    debug(gameTime+" "+playerData[me].name+": "+s)
}
function dumbug(x) {
    const fname = debugGetCallerFuncName()
    debug(`${"dumb".padEnd(8)}|${Date().slice(16, 24)}: [${fname}] ${JSON.stringify(x)}`)
}


const _PATH="dumbbot/"
include(_PATH + "argParse.js")
const argv = argParse()
var dumbSchema = argv.schema==argEnum.schema.RANDOM ? Math.floor(Math.random() * 5) : argv.schema

include(_PATH+"EXT_DACTION.js")
include(_PATH+"asStats.js")

include(_PATH+"LIB.js")
//The bot
include(_PATH+"dumbComm.js")
include(_PATH+"dumbProduction.js")
include(_PATH+"dumbResearch.js")
include(_PATH+"dumbTank.js")
include(_PATH+"dumbTruck.js")

const mainList=[dumbState,dumbProduction,dumbResearch,dumbTank,dumbTruck,dumbComm]
mainList.forEach((i,index)=>setTimer(i.name,500+index+me))

function eventChat(from, to, message="")
{
    if (to!=me)return
    if (!(argv.flags & argEnum.flags.ENABLE_CHATCMD)) return
    if (message[0]!="!")return

    message=message.slice(1)

    if (message[0].toLowerCase()=="s")
    {
        var i=Number(message[1])
        if (isNaN(i)) return chat(ALL_PLAYERS,`not a number:${message[1]}`)
        dumbSchema=i
        chat(ALL_PLAYERS,`schema: ${i}`)
    }
    if (message[0].toLowerCase()=="e")
    {
        argv.reserved ^= argEnum.reserved.HOLD
        chat(ALL_PLAYERS,`Disable droid control: ${!!(argv.reserved & argEnum.reserved.HOLD)}`)
    }
}

function eventStartLevel() 
{
    if (isMultiplayer || !DEBUG)return

    debug("DEBUG")
    /** Add research to observe ally research */
    players.forEach(i=>{
        if (playerData[i].isHuman && !isSpectator(i) && allianceExistsBetween(me,i))
        {
            var struct="A0ResearchFacility"
            var truck=enumDroid(i,DROID_CONSTRUCT)[0]
            var pos=pickStructLocation(truck,struct,truck.x,truck.y)
            addStructure(struct,i,pos.x*128,pos.y*128)
        }
    })
}
