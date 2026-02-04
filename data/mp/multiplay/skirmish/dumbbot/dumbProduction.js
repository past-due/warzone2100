const _cyborgBaseL=["CyborgLightBody",'CyborgLegs',0,0] 
const _cyborgBaseH=["CyborgHeavyBody",'CyborgLegs',0,0]


const templateBody={
    //red4, red3, black3, black2, blue3, yellow3, grey3, blue2, yellow2, grey2, black1, blue1, yellow1, grey1
    HEAVY:["Body14SUP","Body13SUP","Body10MBT","Body7ABT","Body9REC","Body12SUP","Body11ABT","Body6SUPP","Body8MBT","Body5REC","Body3MBT","Body2SUP","Body4ABT","Body1REC"],
    //red4, red3, black3, black2, blue3, blue2, yellow3, grey3, black1, blue1, yellow2, grey2, yellow1, grey1
    COMPACT:["Body14SUP","Body13SUP","Body10MBT","Body7ABT","Body9REC","Body6SUPP","Body12SUP","Body11ABT","Body3MBT","Body2SUP","Body8MBT","Body5REC","Body4ABT","Body1REC"],
    //red4, black2, black1, blue2, yellow2, blue1, grey2, yellow1, grey1
    VTOL:["Body14SUP","Body7ABT","Body3MBT","Body6SUPP","Body8MBT","Body2SUP","Body5REC","Body4ABT","Body1REC"],

    MEDIUM:["Body7ABT","Body6SUPP","Body8MBT","Body5REC","Body3MBT","Body2SUP","Body4ABT","Body1REC"],
    LIGHT:["Body3MBT","Body2SUP","Body4ABT","Body1REC"],
    VLIGHT:["Body4ABT","Body1REC"],
}

const templateProp={
    //red4, red3, black3, black2, blue3, yellow3, grey3, blue2, yellow2, grey2, black1, blue1, yellow1, grey1
    HEAVY:["tracked01","HalfTrack","wheeled01"],
    MEDIUM:["HalfTrack","wheeled01"],
    LIGHT:["hover01","wheeled01"],
    VTOL:["V-Tol"]
}

const templateWeapon={
    MG:["HeavyLaser","Laser2PULSEMk1","Laser3BEAMMk1","MG5TWINROTARY","MG4ROTARYMk1","MG3Mk1","MG2Mk1","MG1Mk1"],
    CANNON:["RailGun3Mk1","RailGun2Mk1",'RailGun1Mk1', "Laser4-PlasmaCannon","Cannon6TwinAslt",'Cannon375mmMk1',"Cannon5VulcanMk1","Cannon4AUTOMk1","Cannon2A-TMk1","Cannon1Mk1"],
    ROCKET:["Missile-A-T","Rocket-HvyA-T","Rocket-LtA-T","Rocket-Pod"],
    ROCKET2:["Missile-MdArt","Rocket-MRL-Hvy","Rocket-MRL"],
    MORTAR:["Howitzer150Mk1","Howitzer-Incendiary","Howitzer03-Rot","Howitzer105Mk1", "Mortar-Incendiary","Mortar3ROTARYMk1","Mortar2Mk1","Mortar1Mk1"],
    FLAME:["PlasmiteFlamer","Flame2","Flame1Mk1"],
    BOMB:["Bomb5-VTOL-Plasmite","Bomb4-VTOL-HvyINC","RailGun2-VTOL","Bomb2-VTOL-HvHE","RailGun1-VTOL","Bomb1-VTOL-LtHE","Cannon4AUTO-VTOL"],
    REPAIR:["HeavyRepair","LightRepair1"],
    SENSOR:["Sensor-WideSpec","SensorTurret1Mk1","Sys-CBTurret01"],
    HPVCAN:["RailGun3Mk1","RailGun2Mk1",'RailGun1Mk1', "Laser4-PlasmaCannon","Cannon6TwinAslt",'Cannon375mmMk1',"Cannon4AUTOMk1","Cannon2A-TMk1","MG3Mk1","Cannon1Mk1"],
}

const _droidAA=["Missile-HvySAM","AAGunLaser","Missile-LtSAM","AAGun2Mk1Quad","QuadRotAAGun","QuadMg1AAGun","AAGun2Mk1","Rocket-Sunburst"]
const _droidAA2=["Missile-HvySAM","AAGun2Mk1Quad","Missile-LtSAM","AAGunLaser","QuadRotAAGun","AAGun2Mk1","QuadMg1AAGun","Rocket-Sunburst"]

const cyborgWeapon={
    MG:["Cyb-Hvywpn-PulseLsr","Cyb-Wpn-Laser","CyborgRotMG","CyborgChaingun"],
    CANNON:["Cyb-Hvywpn-RailGunner","Cyb-Wpn-Rail1","Cyb-Hvywpn-HPV","Cyb-Hvywpn-Mcannon","CyborgCannon","CyborgChaingun"],
    ROCKET:["Cyb-Hvywpn-A-T","Cyb-Hvywpn-TK","CyborgRocket","CyborgChaingun"],
    FLAME:["Cyb-Wpn-Thermite","CyborgFlamer01"],
    MORTAR:["Cyb-Wpn-Grenade"]
}










var monoFactory_lastSensor=0
function monoFactory(factory)
{
    var weaponTeam=unique(enumDroid(me,DROID_WEAPON).concat(enumDroid(me,DROID_CYBORG)),i=>i.id)
    var weaponTeam2=unique(enumDroid(enemy,DROID_WEAPON).concat(enumDroid(enemy,DROID_CYBORG)),i=>i.id) 
    var vtolCount=max(players.filter((i)=>!allianceExistsBetween(me,i) || playerData[i].team!=playerData[me].team).map((i)=>enumDroid(i,DROID_WEAPON).filter(droid=>droid.isVTOL).length))

    //Truck
    if ((
        structurePend && playerPower(me)>500 &&
        enumDroid(me,DROID_CONSTRUCT).length<getDroidLimit(me,DROID_CONSTRUCT)-countStruct("A0LightFactory",me)-1
        ) || enumDroid(me,DROID_CONSTRUCT).length<4
        )
        return buildDroid(factory,"Truck",templateBody.VLIGHT,templateProp.LIGHT,0,0,"Spade1Mk1")

    //Repair
    if (
        countStruct("A0RepairCentre3",me)==0 &&
        weaponTeam.filter(i=>i.health<60).length*.5>enumDroid(me,DROID_REPAIR).length &&
        componentAvailable(undefined,"LightRepair1") &&
        playerPower(me)>100
        )
        return buildDroid(factory,"Repair",templateBody.VLIGHT,templateProp.LIGHT,0,0,["HeavyRepair","LightRepair1"])

    //Sensor for missile
    if (
        //componentAvailable(undefined,"Missile-A-T") &&
        gameTime-monoFactory_lastSensor>60000 &&
        enumDroid(me,DROID_SENSOR).length==0
        )
    {
        monoFactory_lastSensor=gameTime
        return buildDroid(factory,"Sensor",templateBody.LIGHT,templateProp.HEAVY,0,0,"SensorTurret1Mk1")
    }

    //AA
    if (vtolCount>0 && enumDroid(me,DROID_WEAPON).filter(droid=>droid.canHitAir && (droid.body=="Body14SUP" || !droid.canHitGround)).length<vtolCount*.6)
    {
        var w3=((vtolCount>15 && dumbSchema==SCHEMA_CANNON)?_droidAA2:_droidAA).find(x=>componentAvailable(undefined,x))
        
        if (w3!==undefined)return buildDroid(factory,"AA",templateBody.HEAVY,templateProp.HEAVY,0,0,w3)
    }


    var weap=templateWeapon.CANNON
    var body=templateBody.HEAVY
    var prop=(attackedByArtillery>50) ? templateProp.HEAVY : templateProp.MEDIUM

    switch (dumbSchema)
    {
        case SCHEMA_MACGUN:
            var weap=templateWeapon.MG
            break
        case SCHEMA_HPVCAN:
            body=templateBody.COMPACT
            if (!componentAvailable(undefined,"Body5REC")) {
                if (gameTime<5e3) {
                    weap=templateWeapon.REPAIR
                    body=templateBody.VLIGHT
                    prop=templateProp.LIGHT
                }
                else {
                    weap=templateWeapon.MG
                }
            }
            else {
                var weap=templateWeapon.HPVCAN
            }
            break         
        case SCHEMA_CANNON:
            var weap=templateWeapon.CANNON
            break

        case SCHEMA_ROCKET:
            body=templateBody.COMPACT

            var antiTankTeam=weaponTeam.filter(i=>objectWeaponStat(i).Effect=="ANTI TANK")
            var antiCyborgTeam=weaponTeam.filter(i=>objectWeaponStat(i).Effect=="ARTILLERY ROUND")
            var enemyTankTeam=weaponTeam2.filter(i=>i.droidType==DROID_WEAPON && i.propulsion!="CyborgLegs")
            var enemyCyborgTeam=weaponTeam2.filter(i=>i.droidType==DROID_CYBORG)
            
            var antiTankCount=antiTankTeam.map(i=>i.propulsion=="CyborgLegs"?i.body=="CyborgLightBody"?.5:.75:1).reduce((a,b)=>(a+b),0)
            var antiCyborgMove=antiCyborgTeam.filter(i=>objectWeaponStat(i).FireOnMove)

            var antiTankToomuch=antiTankCount/(1+enemyTankTeam.length) > 1.2*antiCyborgTeam.length/(1+enemyCyborgTeam.length)
            if (!antiTankToomuch) {
                var weap=templateWeapon.ROCKET
            }
            else if ( componentAvailable(undefined,"Missile-MdArt") ||
                (antiCyborgMove.length/(1+antiCyborgTeam.length))<.5 && templateWeapon.ROCKET2.some(i=>componentAvailable(undefined,i)))
            {
                weap=templateWeapon.ROCKET2
            }
            else if (!componentAvailable(undefined,"Body5REC") && countDroid(DROID_ANY,me)<15)
            {
                weap=templateWeapon.MG
            }
            else if (countDroid(DROID_ANY,me)<30)
            {
                weap=templateWeapon.FLAME
            }
            else {
                weap=templateWeapon.MORTAR
                if (!componentAvailable(undefined,"Mortar3ROTARYMk1"))
                {
                    body=templateBody.VLIGHT
                    prop=templateProp.LIGHT
                }
            }
            break

        case SCHEMA_MORTAR:
            var weap=templateWeapon.MORTAR
            break
        case SCHEMA_FLAMER:
            var weap=templateWeapon.FLAME
            break
        default:
            var weap=templateWeapon.CANNON
            break
    }

    var name=weap.find(i=>componentAvailable(undefined,i))
    return buildDroid(factory,name,body,prop,0,0,weap,weap)
}
function monoFactory2(factoryCyborg)
{
    switch (dumbSchema)
    {
        case SCHEMA_MACGUN:
            var w=cyborgWeapon.MG.find(x=>componentAvailable(undefined,x))
            break
        case SCHEMA_HPVCAN:
            if (
                componentAvailable(undefined,"Cannon4AUTOMk1") &&
                !componentAvailable(undefined,'RailGun1Mk1') &&
                !componentAvailable(undefined,"Cyb-Wpn-Laser") &&
                !componentAvailable(undefined,"Cyb-Hvywpn-Mcannon") &&       
                flamerRatio<.5 &&
                playerPower(me) < 1500
                )return
            if (componentAvailable(undefined,"Cyb-Wpn-Laser") && !getResearch("R-Wpn-Rail-Damage01",me).done)
            {
                var w="Cyb-Wpn-Laser"
            }
            else
            {
                if (!componentAvailable(undefined,"Cyb-Hvywpn-Mcannon")) var w=cyborgWeapon.MORTAR.find(x=>componentAvailable(undefined,x))
                else var w=cyborgWeapon.CANNON.find(x=>componentAvailable(undefined,x))
                if (!componentAvailable(undefined,"Cannon4AUTOMk1") && Math.random()<.2*(1+2*flamerRatio)) w="CyborgFlamer01"//if (urandom()<.3*(1+2*flamerRatio)) w="CyborgFlamer01"
            }
            break            
        case SCHEMA_CANNON:
            var w=cyborgWeapon.CANNON.find(x=>componentAvailable(undefined,x))
            if (!componentAvailable(undefined,"Cannon375mmMk1") && Math.random()<.25) w="CyborgFlamer01"
            break
        case SCHEMA_ROCKET:
            var w=cyborgWeapon.ROCKET.find(x=>componentAvailable(undefined,x))
            if (!componentAvailable(undefined,"Rocket-LtA-T")) if (Math.random()<.3*(1+2*flamerRatio)) w="CyborgFlamer01"//if (urandom()<.3*(1+2*flamerRatio)) w="CyborgFlamer01"
            break
        case SCHEMA_MORTAR:
            var w=cyborgWeapon.FLAME.find(x=>componentAvailable(undefined,x))
            break
        case SCHEMA_FLAMER:
            var w=cyborgWeapon.FLAME.find(x=>componentAvailable(undefined,x))
            break       
    }

    if (structurePend && countDroid(DROID_CONSTRUCT,me)<getDroidLimit(me,DROID_CONSTRUCT)-countStruct("A0CyborgFactory",me)-3)w="CyborgSpade"
    if (w==undefined)return
    if (componentAvailable(undefined,"Body10MBT") && componentAvailable(undefined,"RailGun3Mk1"))return
    
    if (w.includes("Hvywpn"))return buildDroid(factoryCyborg,w,"CyborgHeavyBody",'CyborgLegs',0,0,w)
    return buildDroid(factoryCyborg,w,"CyborgLightBody",'CyborgLegs',0,0,w)
}

function monoFactory3(factoryVtol)
{
    var w=templateWeapon.BOMB
    var w2=w
    if (!w.find(x=>componentAvailable(undefined,x)))return
    if (Math.random()<.2 && componentAvailable(undefined,"Bomb6-VTOL-EMP"))w=["Bomb6-VTOL-EMP"]
    var name=w.find(i=>componentAvailable(undefined,i))
    buildDroid(factoryVtol,name,templateBody.VTOL,templateProp.VTOL,0,0,w,w2)
}

function dumbProduction()
{
    var cyborg_factory=enumStruct(me,CYBORG_FACTORY)
    var factory=enumStruct(me,FACTORY)
    var vtol_factory=enumStruct(me,VTOL_FACTORY)
    var power=playerPower(me)-queuedPower(me)
    const q=factory.filter(structureIdle)
    const q2=cyborg_factory.filter(structureIdle)   
    /*
    if ((dumbSchema!==SCHEMA_ROCKET) || (enumDroid(me,DROID_CYBORG).length<10+enumDroid(me,DROID_WEAPON).length*2))
    {
        if (playerPower(me)>100)cyborg_factory.filter(structureIdle).forEach(monoFactory2)
    }*/
    if (power>150*q.length )q.forEach(monoFactory)
    power-=200*q.length
    if (power>(mortarRatio<.5 ? 100 : 1500)+150*q2.length)q2.forEach(monoFactory2)
    power-=100*q2.length
    if (power> 1500)vtol_factory.filter(structureIdle).forEach(monoFactory3) 
}
