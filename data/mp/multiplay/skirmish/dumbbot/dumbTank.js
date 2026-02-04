
var pos=startPositions[me]//target
var rpos=home//repair centre
var state=0

var repairThreshold=40
var _repairMod=[1, 1.2, 1.5, 1, 1, 1, 1]
var repairHigh=.5
var repairLow=.1

var limitHigh=.9
var limitLow=.7


var teamVHigh=2
var teamHigh=1.35
var teamLow=.9

var enemyDistance=Infinity
var enemyHigh=[48,48,54,48,48,48,48]
var enemyLow=[32,32,36,32,32,32,32]
var enemy=me
var enemyPrev=enemy
var psi=false
var _weaponRange=1
var _weaponRangeRatio=1

var teamRatio=1.0
var flamerRatio=0.0
var indirectRatio=0.0
var rocketRatio=0.0
var mortarRatio=0.0
var distRatio=1.0

function _getEnemyHigh()
{
    return enemyHigh[dumbSchema]*Math.max(1, _weaponRangeRatio)
}
function _getEnemyLow()
{
    return enemyLow[dumbSchema]*Math.max(1, _weaponRangeRatio)
}

/**
 * return all tanks and cyborgs without repeat\
 * (super cyborg counted twice at)\
 * wzapi.cpp: line 1079
 * @param {Number} player 
 */
function enumArmy(player=me)
{
    return enumDroid(player,DROID_WEAPON).filter(i=>i.propulsion!="CyborgLegs" && !isVTOL(i)).concat(enumDroid(player,DROID_CYBORG))
}

function enumVtol(player=me)
{
    return enumDroid(player,DROID_WEAPON).filter(i=>isVTOL(i))
}


function dumbState()
{
    /*
    if (countDroid(DROID_ANY,me)>0 && enemy!==me)
    {
        hackMarkTiles()
        var dp=max(enumDroid(enemy),distTo(home))
        enumDroid(enemy).forEach(i=>{
            var pos=interceptPos(home,i,dp)
            hackMarkTiles(pos.x,pos.y)
        })
    }*/

    if (countStruct("A0RepairCentre3",me)>0)rpos=enumStruct(me,"A0RepairCentre3")[Math.floor(enumStruct(me,"A0RepairCentre3").length*.99*Math.random())]
    else if (attackPath.length>0) rpos=attackPath[Math.floor(attackPath.length*.15)]

    /**
     * @param {_droid | _struct} obj
     * @param {_pos} pos 
     */
    function realDistance(obj,pos)
    {
        if (obj.type==DROID) return distBetween(obj,pos)/2
        return distBetween(obj,pos)
    }

    var weaponTeam=enumArmy(me)
    _weaponRange=weaponRange(me)
    _weaponRangeRatio=_weaponRange/weaponRange(enemy)

    enemy=max(
        playerData.map((data,i)=>i).filter(i=>!isSpectator(i) && !allianceExistsBetween(me,i)),
        i=>countDroid(DROID_ANY,i)+enumStruct(enemy).filter(i=>propulsionCanReach("wheeled01",home.x,home.y,i.x,i.y)).map(i=>i.cost===undefined?0:i.cost/300).reduce((a,b)=>a+b,0)
    )
    dumbStateDaemon()

    if (weaponTeam.length==0)return
    var weaponTeam2=enumArmy(enemy)

    var limitRatio=(dumbSchema==SCHEMA_HPVCAN?1.25:1) * countDroid(DROID_ANY,me) / (oilRatio>20?getDroidLimit(me):16+oilRatio*2)
    var repairRatio=weaponTeam.filter(weapon=>weapon.health<repairThreshold*_repairMod[dumbSchema]).length/(1+weaponTeam.length)
    if (psi)repairRatio=0

    var enemyTeam3=enumDroid(enemy).filter(i=>i.propulsion!="CyborgLegs" && droidCanReach(weaponTeam[0],i.x,i.y))
    psi=(oilRatio<24 && weaponTeam.length/weaponTeam2.length>teamVHigh && weaponTeam.length>50)
    if (enemyTeam3.length==0 || psi)
    {
        enemyTeam3=enumStruct(enemy).filter(i=>droidCanReach(weaponTeam[0],i.x,i.y))
        if (psi)
        {
            enemyTeam3=enemyTeam3.filter(i=>i.cost>25)
        }
    }

    if (psi)
    {
        var enemyNearest=min(enemyTeam3,distTo(pos))
    }
    else
    {
        var enemyNearest=min(enemyTeam3,i=>realDistance(i,home)+realDistance(i,pos))
    }
    enemyDistance=distBetween(enemyNearest,rpos)

    var vsize=Math.max(35+enemyDistance/5,5+weaponTeam2.length*(1-indirectRatio*.3))
    distRatio=enemyTeam3.length>0 ? Math.min(2,distBetween(enemyNearest,rpos)/distBetween(enemyNearest,startPositions[enemy])) : 1

    if (weaponTeam2.length>0)
    {
        flamerRatio=weaponTeam2.filter(weapon=>objectWeaponStat(weapon).ImpactClass=="FLAME").length/(1+weaponTeam2.length)
        mortarRatio=weaponTeam2.filter(weapon=>objectWeaponStat(weapon).ImpactClass=="MORTARS").length/(1+weaponTeam2.length)
        indirectRatio=weaponTeam2.filter(weapon=>weapon.hasIndirect).length/(1+weaponTeam2.length)
        rocketRatio=weaponTeam2.filter(weapon=>(objectWeaponStat(weapon).ImpactClass=="ROCKET" || objectWeaponStat(weapon).ImpactClass=="MISSILE")).length/(1+weaponTeam2.length)
    }
    else
    {
        weaponTeam2=enemyTeam3.slice(0,10)
    }

    if (enemyDistance>_getEnemyHigh())
    {
        teamRatio=weaponTeam.length/vsize
    }
    else
    {
        teamRatio=weaponTeam.filter(weapon=>(distBetween(weapon,rpos)>(enemyDistance-24))).length/vsize
    }
    teamRatio*=(1-rocketRatio*.25)

    if (gameTime-ptime>5000)
    {
        if (countDroid(DROID_ANY,enemy)>0 && teamRatio<teamVHigh) pos=enemyNearest
        else 
        {
            var enemyStruct=enumStruct(enemy).filter(i=>propulsionCanReach("wheeled01",home.x,home.y,i.x,i.y))
            pos=enemyStruct.length>0?min(enemyStruct,i=>distBetween(pos,i)):rpos
        }

    }

    if (state==0)//defense
    {
        if ((enemyDistance < _getEnemyLow()) ||
            (limitRatio > limitHigh && repairRatio < repairLow) ||
            (teamRatio > teamHigh)
        ) state=1
    }
    if (state==1)//attack
    {
        if ((enemyDistance > _getEnemyHigh()) &&
            (limitRatio < limitLow || repairRatio > repairHigh) &&
            (teamRatio < teamLow)
        ) state=0
    }
}

var dumbStateDaemon_pathfindStart=0
var dumbStateDaemon_pathfindDone=true
function dumbStateDaemon()
{
    if (enemy!==enemyPrev)
    {
        dumbStateDaemon_pathfindStart=gameTime
        dumbStateDaemon_pathfindDone=false
        enemyPrev=enemy
    }

    if (!dumbStateDaemon_pathfindDone)
    {
        var anchor=min(enumDroid(me),distTo(home))
        orderDroidLoc(anchor,DORDER_MOVE,startPositions[enemy].x,startPositions[enemy].y)
        if (gameTime>dumbStateDaemon_pathfindStart+1200)
        {
            attackPath.splice(0)
            attackPath.push(...getDroidPath(anchor))
            dumbStateDaemon_pathfindDone=true
            orderDroidLoc(anchor,DORDER_MOVE,home.x,home.y)
        }
    }   
}




var ptime=0
var headRatio=.5// unit ratio
var headThreshold=10// distance

function dumbTank()
{
    var weaponTeam=enumArmy(me)
    var vtolTeam=enumVtol(me)
    var sensorTeam=enumDroid(me,DROID_SENSOR)
    //var enemyTeam=enumDroid(enemy,DROID_WEAPON).concat(enumDroid(enemy,DROID_CYBORG))

    if (weaponTeam.length==0)return

    var headArg=argsort(weaponTeam,weapon=>distBetween(weapon,pos))
    var headBack=distBetween(weaponTeam[headArg[ Math.floor(weaponTeam.length*headRatio) ]],pos)

    var healthThreshold=weaponTeam.sort(by(i=>i.health))[Math.floor(weaponTeam.length*.2)].health

    function retreat(droid,odd=0,coef=5)
    {
        if (!FLATMAP && distBetween(droid,home)<20)
        {
            if (countStruct("A0RepairCentre3",me)>0)return orderDroid(droid,DORDER_RTR)
            return orderDroidLoc(droid,DORDER_MOVE,rpos.x,rpos.y)
        }

        var target=rpos
        if (weaponTeam.length>35 && oilRatio>24 && droid.health>30)
        {
            var coef2=(dumbSchema==SCHEMA_CANNON)?4:2.5
            var pos2=yawTo(droid,target,coef2*odd,coef)
            if (droidCanReach(droid,pos2.x,pos2.y))return orderDroidLoc(droid,DORDER_MOVE,pos2.x,pos2.y)
        }
        if (countStruct("A0RepairCentre3",me)>0)return orderDroid(droid,DORDER_RTR)
        return orderDroidLoc(droid,DORDER_MOVE,rpos.x,rpos.y)
    }

    function attack(droid,odd=0,coef=5,order=DORDER_MOVE)
    {
        if (!FLATMAP && distBetween(droid,home)<20)return orderDroidLoc(droid,order,pos.x,pos.y)

        var target=pos
        var coef2=(dumbSchema==SCHEMA_CANNON)?1:1.4
        var stat=objectWeaponStat(droid)

        if (!stat.FireOnMove && stat.Effect=="ARTILLERY ROUND" && sensorTeam.length>0 && sensorTeam[0].action==DACTION_OBSERVE)
        {
            return orderDroidObj(droid,DORDER_FIRESUPPORT,sensorTeam[0])
        }

        var pos2=yawTo(droid,target,coef2*odd,coef)
        if (droidCanReach(droid,pos2.x,pos2.y))return orderDroidLoc(droid,order,pos2.x,pos2.y)
        var pos2=yawTo(droid,target,0,coef*1.5)
        if (droidCanReach(droid,pos2.x,pos2.y))return orderDroidLoc(droid,order,pos2.x,pos2.y)

        return retreat(droid,-odd,coef)
    }

    if (argv.reserved & argEnum.reserved.HOLD)return

    var lr=0
    if (state==0) weaponTeam.forEach((droid,i)=>
    {
        if (distBetween(droid,rpos)>_getEnemyLow()-18) return retreat(droid)
        if (droid.health<Math.min(95,healthThreshold)) return retreat(droid)
        //if (enumRange(rpos.x,rpos.y,8,ALLIES).filter(i=>i.weapons.length>0).length<weaponTeam.length*.3)return retreat(droid)
        if (
            enemyDistance>_getEnemyHigh() &&
             distBetween(droid,home)>_getEnemyLow()-18+((droid.propulsion=="CyborgLegs")?4:0))return

        var odd=droid.id%3-1
        return attack(droid,-odd,5,DORDER_MOVE)
    })

    if (state==1) weaponTeam.forEach((droid,i)=>
    {
        //weapon stats
        var stat=objectWeaponStat(droid)
        //when move spread droids to left | middle | right
        var odd=droid.id%3-1

        //recycle old droid
        if (lr<5 && droid.born<gameTime*1/2 && countDroid(DROID_ANY,me)>getDroidLimit(me)-5)
        {
            lr+=1
            return orderDroid(droid,DORDER_RECYCLE)
        }
        //repair damaged droid
        if (!psi && droid.health<repairThreshold*_repairMod[dumbSchema])
        {
            return retreat(droid,odd,5)
            //if (countStruct("A0RepairCentre3",me)>0)return orderDroid(droid,DORDER_RTR)
            //return orderDroid(droid,DORDER_RTB)
        }
        //retreat single droid at front
        //retreat AA at front
        if (!psi)
        {
            if (indirectRatio<.5 && (headBack-distBetween(droid,pos) > headThreshold))return retreat(droid,odd,2)
            if (!droid.canHitGround && (headBack+stat.MaxRange/384-distBetween(droid,pos) > headThreshold))return retreat(droid,odd)
        }

        //TODO: retreat units by unit type
        //if army not overpowered
        if (1)
        {   
            // BUG:
            // the bug affects all droid that weapons range shorter than sensor

            // DORDER_PATROL make droid hold fire and chase best target it see
            // until the target get in range
            // because previous script retreats droid when its weapon fired,
            // when target hides behind, the droid will rush into enemy and get destroyed

            // fixed by order a droid to move back when any enemy in range,
            // DORDER_MOVE make droid attack target in range immediately

            //reduce expose of short ranged pulse weapon 
            //cyborg spin fast
            var margin=stat.FireOnMove?0:droid.propulsion=="CyborgLegs"?2:8
            var pressure=stat.FireOnMove?0:droid.propulsion=="CyborgLegs"?1:4      
            if (
                (enumRange(droid.x,droid.y,stat.MaxRange/128-margin,ENEMIES).length>pressure)
                )return retreat(droid,odd)

        }
        

        function mode()
        {
            //if (oilRatio<24 && weaponTeam.length<30)return DORDER_PATROL
            if (!stat.FireOnMove)return DORDER_PATROL
            if (droid.name=="CMD")return DORDER_PATROL          
            if (distBetween(droid,pos)>20)return DORDER_MOVE
            if (stat.ImpactClass=="GAUSS" || stat.Id=="Cannon375mmMk1")return DORDER_PATROL
            if (dumbSchema==SCHEMA_CANNON && rocketRatio>.6)return DORDER_MOVE
            if (stat.ReloadTime>0 && (
                //weapon.propulsion=="CyborgLegs" ||
                stat.ImpactClass=="MORTARS" ||
                stat.ImpactClass=="GAUSS" ||
                stat.ImpactClass=="ENERGY" ||
                stat.ImpactClass=="ROCKET" ||
                stat.ImpactClass=="MISSILE"
            ))return DORDER_PATROL
            return DORDER_MOVE
        }
        attack(droid,-odd,undefined,mode())
    })

    var sensorTeamRange=Upgrades[me].Sensor["Sensor Turret"].Range/128-2
    sensorTeam.forEach(droid=>{
        if (droid.health<60 || enumRange(droid.x,droid.y,sensorTeamRange,ENEMIES).length>0)return orderDroid(droid,DORDER_RTR)
        return orderDroidLoc(droid,DORDER_PATROL,pos.x,pos.y)
    })

    function rearming(droid)
    {
        return [DACTION_MOVETOREARMPOINT,DACTION_MOVETOREARM,DACTION_CLEARREARMPAD,DACTION_WAITFORREARM,DACTION_WAITDURINGREARM].some(i=>droid.action==i)
    }
    lr=0
    vtolTeam=vtolTeam.filter(i=>!rearming(i))
    vtolTeam.forEach(droid=>{
        if (droid.born<gameTime*1/2 && countDroid(DROID_ANY,me)>getDroidLimit(me)-5)
        {
            if (lr<5)
            {
                lr+=1
                return orderDroid(droid,DORDER_RECYCLE)
            }
        }

        if (droid.weapons[0].armed==0 || droid.health<40 || (distBetween(pos,home)>20 && vtolTeam.length<10))
        {
            return orderDroid(droid,DORDER_REARM)
        }
        
        return orderDroidLoc(droid,DORDER_PATROL,pos.x,pos.y)
    })

    var repairTeam=enumDroid(me,DROID_REPAIR)
    repairTeam.forEach(droid=>{
        orderDroidLoc(droid,DORDER_PATROL,rpos.x,rpos.y)
    })
}


/** An event that is run when an object belonging to the script's controlling player is
attacked. The attacker parameter may be either a structure or a droid. 
* @param {_droid | _struct} victim
* @param {_droid | _struct} attacker
*/
function eventAttacked(victim, attacker) {
    /** @type {_weaponStat} */
    if (allianceExistsBetween(me,attacker.player)) return
    if (!allianceExistsBetween(me,victim.player)) return

    const weap=Stats.Weapon[attacker.weapons[0].fullname]
    if ((!weap.FireOnMove) && weap.Effect=="ARTILLERY ROUND"){
        attackedByArtillery+=1
    }
}