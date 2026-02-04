//#region BEGIN
function goodResearch(id="")
{
    var cost=getResearch(id,me).power
    var power=playerPower(me)-queuedPower(me)-enumStruct(me,FACTORY).filter(i=>structureIdle(i)).length*400
    
    const HIGH=-10000
    const NORMAL=0
    const LOW=10000
    
    function toward(researchName)
    {
        return findResearch(researchName).map(i=>i.id).includes(id)
    }
    function reached(researchName,player=me)
    {
        var res=getResearch(researchName,player)
        return res.started || res.done 
    }
    function done(researchName,player=me)
    {
        return getResearch(researchName,player).done 
    }
    function prefix(researchName)
    {
        return id.startsWith(researchName) 
    }

    if (getResearch(id,me).started)return Infinity
    
    //early
    if (toward("R-Vehicle-Prop-Halftracks"))return HIGH-200     
    if (toward("R-Wpn-MG-Damage01"))return HIGH-100
    if (!done("R-Struc-Factory-Module"))
    {
        if (toward("R-Struc-Research-Upgrade09"))return HIGH-200
        if (toward("R-Struc-Factory-Module"))return HIGH-200
        if (toward("R-Wpn-MG2Mk1"))return HIGH+100         
        if (toward("R-Wpn-Flamer-Damage01"))return HIGH+150
        if (toward("R-Struc-RepairFacility"))return NORMAL+cost+100
    }
    else
    {
        if (toward("R-Struc-RepairFacility"))return countDroid(DROID_ANY,me)>15?HIGH-200:HIGH+cost
    }
    if (!reached("R-Vehicle-Metals01"))
    {
        if (prefix("R-Vehicle-Engine02"))return LOW+cost+1000
    }

    //core
    if (prefix("R-Sys-Autorepair-General"))return HIGH-2000
    if (toward("R-Struc-Research-Upgrade09"))return HIGH-1000

    //armor & body

    if (toward("R-Vehicle-Prop-Halftracks"))return HIGH-50

    if (dumbSchema==SCHEMA_HPVCAN)
    {
        if (prefix("R-Struc-Factory-Upgrade"))return HIGH-50
        if (toward("R-Vehicle-Body05"))return HIGH-10 
        if (toward("R-Vehicle-Body06"))return HIGH+10
        if (toward("R-Vehicle-Metals09"))return HIGH+20 
        if (toward("R-Vehicle-Body09"))return HIGH+30
        if (reached("R-Wpn-Cannon4AMk1"))
        {
            if (prefix("R-Sys-Sensor-Upgrade01"))return HIGH-10 
        }

        if (reached("R-Wpn-RailGun01"))
        {
            if (toward("R-Cyborg-Metals09"))return HIGH+cost-200
            if (prefix("R-Cyborg-Hvywpn") && toward("R-Cyborg-Hvywpn-RailGunner"))return HIGH+cost-300
        }
        else
        {
            if (toward("R-Wpn-Mortar-ROF01"))return HIGH+cost-5
            if (toward("R-Cyborg-Metals04"))return HIGH+cost+45  
            if (reached("R-Cyborg-Metals04") && toward("R-Cyborg-Hvywpn-HPV"))return HIGH+cost-200
        }

        if (prefix("R-Vehicle-Prop-Tracks"))return (reached("R-Vehicle-Body10"))?HIGH:LOW
    }
    else
    {
        if (true || (mortarRatio<.5 && attackedByArtillery<50))
        {
            if (prefix("R-Vehicle-Prop-Tracks"))return (reached("R-Vehicle-Body10"))?HIGH:LOW
            if (toward("R-Vehicle-Body11"))return HIGH+cost-70
            if (toward("R-Vehicle-Metals04"))return HIGH+cost-70
            if (toward("R-Cyborg-Metals04"))return HIGH+cost-200
            if (prefix("R-Cyborg-Hvywpn"))return HIGH+cost-200
            if (toward("R-Cyborg-Metals06"))return NORMAL+cost-120
            if (toward("R-Vehicle-Metals09"))return NORMAL+cost-120
            if (toward("R-Cyborg-Metals09"))return NORMAL+cost+1
        }
        else
        {
            if (prefix("R-Vehicle-Prop-Tracks"))return HIGH
            if (toward("R-Vehicle-Body11"))return HIGH+cost-480
            if (toward("R-Vehicle-Metals09"))return NORMAL+cost-320
            if (toward("R-Cyborg-Metals09"))return NORMAL+cost+600
            if (prefix("R-Cyborg-Hvywpn"))return NORMAL+cost
        }
    }

    //util
    if (toward("R-Struc-Factory-Upgrade09"))
    {
        if (cost>100 && power<5000 && !reached("R-Struc-Power-Upgrade01b"))return LOW

        if (prefix("R-Struc-Factory-Upgrade"))
        {
            if (cost<100 && dumbSchema==SCHEMA_ROCKET)return HIGH
            if (cost>100 && !(reached("R-Cyborg-Metals03") || reached("R-Vehicle-Metals03")))return LOW
            return (power>2000)?HIGH:(power>1000)?NORMAL+cost:NORMAL+cost+2500
        }
        return (power>3500)?HIGH:LOW
    }
    if (toward("R-Struc-Power-Upgrade03a"))
    {
        return (power<1000)?HIGH:(power<2000)?NORMAL-50:(power<5000)?NORMAL+cost+500:LOW
        //if (prefix("R-Struc-Power-Upgrade01") && !reached("R-Struc-Factory-Upgrade04"))return (power<500)?HIGH:LOW
    }


    if (prefix("R-Sys-Sensor-Upgrade01"))return HIGH+cost+100
    if (dumbSchema==SCHEMA_CANNON && prefix("R-Struc-RprFac-Upgrade01") && reached("R-Wpn-Cannon2Mk1"))return HIGH+cost


    switch (dumbSchema)
    {
        case SCHEMA_MACGUN:
            if (prefix("R-Wpn-Laser") || toward("R-Wpn-Laser01"))return HIGH
            if (toward("R-Wpn-MG5"))return HIGH
            if (prefix("R-Wpn-MG"))return HIGH+cost+400
            break

        default:
        case SCHEMA_CANNON:
            if (false && toward("R-Defense-WallTower-DoubleAAgun02"))return HIGH
            if (prefix("R-Wpn-Cannon") || prefix("R-Wpn-Rail"))
            {
                if (prefix("R-Wpn-Rail")) return HIGH
        
                if (toward("R-Wpn-RailGun01"))
                {
                    if (prefix("R-Wpn-Cannon-Damage")) return HIGH
                    if (reached("R-Wpn-Cannon-Damage07"))return HIGH// Accuracy 1,2 & HPV Cannon
                    return LOW+100
                }
                if (toward("R-Wpn-Cannon3Mk1")) return HIGH+cost
                if (reached("R-Wpn-RailGun01"))return LOW+cost+200
                return NORMAL+cost+400
            }
            if (toward("R-Wpn-MG3Mk1"))return HIGH
            break

        case SCHEMA_HPVCAN:
            if (toward("R-Wpn-Laser01"))return HIGH+1
            //if (toward("R-Wpn-MG3Mk1"))return HIGH
            if (prefix("R-Wpn-Cannon"))
            {
                if (done("R-Wpn-RailGun01")) return LOW

                if (prefix("R-Wpn-Cannon2Mk1")) return HIGH-1
                if (prefix("R-Wpn-Cannon-ROF")) return HIGH+1
                if (toward("R-Wpn-Cannon4AMk1")) return HIGH-100
                if (toward("R-Wpn-RailGun01"))
                {
                    if (prefix("R-Wpn-Cannon-Damage")) return HIGH
                    if (reached("R-Wpn-Cannon-Damage07"))return HIGH// Accuracy 1,2 & HPV Cannon
                    return LOW+100
                }
                return LOW+100
            }

            if (prefix("R-Wpn-Rail"))
            {
                if (prefix("R-Wpn-RailGun")) return HIGH-100
                return HIGH-50
            }
            break

        case SCHEMA_ROCKET:
            /**
             * Tweaked rocket path, complete Rocket ROF3 before 10min
             */
            //if (toward("R-Wpn-MG3Mk1"))return HIGH
            //if (mortarRatio>.5 && toward("R-Defense-IDFRocket"))return HIGH

            if (reached("R-Vehicle-Prop-VTOL",enemy))
            {
                if (reached("R-Wpn-Missile2A-T") && toward("R-Wpn-Missile-LtSAM")) return HIGH-5
                if (!reached("R-Wpn-Missile2A-T") && toward("R-Defense-Sunburst")) return HIGH-5
            }

            if (reached("R-Wpn-Missile2A-T"))
            {
                if (toward("R-Wpn-MdArtMissile")) return HIGH-100
                if (prefix("R-Sys-Sensor-Upgrade"))return NORMAL+cost+100
            }

            if (prefix("R-Cyborg-Hvywpn-A-T"))return HIGH-100
            if (prefix("R-Wpn-Rocket03-HvAT"))return LOW//Bunker Buster Rocket

            if (toward("R-Wpn-Rocket02-MRL")) return HIGH-46

            if (!reached("R-Wpn-Mortar-Damage02"))
            {
                if (prefix("R-Wpn-Mortar-Damage")) return reached("R-Vehicle-Metals02")?HIGH+5:LOW
            }
            else
            {
                //if (toward("R-Wpn-Mortar-ROF02")) return HIGH-25
                if (toward("R-Wpn-Mortar3"))return HIGH+225   
            }



            if (prefix("R-Wpn-Rocket"))
            {
                if (prefix("R-Wpn-RocketSlow-Accuracy"))return HIGH-70
                if (prefix("R-Wpn-Rocket-Accuracy"))return reached("R-Wpn-Rocket-Damage04")?HIGH-50:LOW
                if (prefix("R-Wpn-Rocket-Damage05"))return reached("R-Wpn-RocketSlow-Accuracy01")?HIGH-50:LOW             
                if (toward("R-Wpn-Missile2A-T"))return HIGH-60

                if (toward("R-Wpn-Rocket-ROF03")) return HIGH-55
                //if (prefix("R-Wpn-Rocket02-MRL")) return HIGH-45
                return NORMAL+cost+200
            }

            if (prefix("R-Wpn-Missile2A-T")) return HIGH-95
            
            if (prefix("R-Wpn-Missile"))
            {
                if (prefix("R-Wpn-Missile-LtSAM")) return NORMAL+cost+400
                if (prefix("R-Wpn-Missile-Accuracy")) return LOW-200
                if (prefix("R-Wpn-Missile-ROF"))return HIGH+cost-400             
                return HIGH+cost
            }
            break

        case SCHEMA_MORTAR:
            //if (toward("R-Wpn-MG3Mk1"))return HIGH
            if (toward("R-Wpn-Mortar3"))return HIGH
            if (toward("R-Defense-MortarPit-Incendiary")) return HIGH+cost+100
            if (toward("R-Wpn-HvyHowitzer")) return HIGH+cost
            if (reached("R-Defense-MortarPit-Incendiary") && toward("R-Defense-HvyHowitzer")) return HIGH
            if (prefix("R-Wpn-Mortar") || prefix("R-Wpn-Howitzer"))
            {
                if (prefix("R-Wpn-Mortar-Acc"))return LOW+cost
                if (prefix("R-Wpn-Howitzer-Accuracy"))return LOW+cost
                if (prefix("R-Wpn-Howitzer-ROF")) return HIGH+cost
                if (toward("R-Wpn-Mortar-ROF02")) return HIGH+cost+100

                return NORMAL+cost+600
            }

        case SCHEMA_FLAMER:
            //if (toward("R-Wpn-MG3Mk1"))return HIGH
            if (toward("R-Defense-PlasmiteFlamer")) return HIGH
            if (prefix("R-Wpn-Flamer-"))return HIGH+150
            if (toward("R-Defense-MortarPit-Incendiary")) return NORMAL+cost
            break
    }

    if (prefix("R-Struc-RprFac-Upgrade"))
    {
        return (toward("R-Vehicle-Metals03"))?NORMAL+cost+300:LOW
    }

    //late research
    if (reached("R-Struc-Research-Upgrade08"))
    {
        if (toward("R-Vehicle-Body09"))return reached("R-Vehicle-Metals05") ? NORMAL-100 : NORMAL+cost+100
        //if (toward("R-Vehicle-Body12"))return NORMAL+cost+200
        if (toward("R-Vehicle-Engine06"))return NORMAL+cost+175
        if (toward("R-Vehicle-Body10"))return NORMAL+cost+300
        if (toward("R-Vehicle-Body14"))return  NORMAL+cost+500

        if (prefix("R-Vehicle-Prop"))return LOW+50
        if (toward("R-Defense-HvyHowitzer"))return LOW+100
        if (toward("R-Wpn-LasSat"))return LOW+150
        if (toward("R-Defense-MassDriver"))return LOW+200
        if (toward("R-Defense-HvyArtMissile"))return LOW+250
        if (toward("R-Defense-WallTower-SamHvy"))return LOW+255   
        if (prefix("R-Sys"))return LOW+300
        if (prefix("R-Struc"))return LOW+300
        
    }

    if (id=="R-Vehicle-Body04")return LOW+300
    return LOW+1000
}


function goodResearch2(id="")
{
    return goodResearch(id)-getResearch(id,me).points/100000
}

//#region END

function monoResearch(lab)
{
    var research=enumResearch()
    if (research.length==0) return

    pursueResearch(lab,min(research,i=>goodResearch2(i.id)).id)
}



function dumbResearch()
{
    var lab=enumStruct(me,RESEARCH_LAB)
    lab.filter(structureIdle).forEach(monoResearch)
}

/** An event that is run whenever a new research is available. The structure
parameter is set if the research comes from a research lab owned by the
current player. If an ally does the research, the structure parameter will
be set to null. The player parameter gives the player it is called for. 
* @param {_research} research
* @param {_struct} structure
* @param {Number} player
*/
/*
function eventResearched(research, structure, player) 
{
    if (player!==me)return
    if (gameTime<100)return
    monoResearch(structure)
}*/
