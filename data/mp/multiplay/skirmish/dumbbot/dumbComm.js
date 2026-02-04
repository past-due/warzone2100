function dumbComm()
{
    oilRatio=derrickPositions.length / players.filter(i=>!isSpectator(i)).length

    //transfer power and truck to allies
    players.forEach(i=>{
        if (allianceExistsBetween(me,i))
        {
            if (playerPower(me)-queuedPower(me)>2000 && playerPower(i)-queuedPower(i)<0)
            {
                donatePower((playerPower(me)-queuedPower(me))*.2,i)
            }
            if (enumDroid(me,DROID_CONSTRUCT).length>8 && enumDroid(i,DROID_CONSTRUCT).length<4)
            {
                enumDroid(me,DROID_CONSTRUCT).slice(0,4).forEach(truck=>donateObject(truck,i))
            }
        }
    })

    //cost of all enemy objects near target (radius=4)
    function costSum(i)
    {
        return sum(enumRange(i.x,i.y,4,ENEMIES).filter(i=>i.type==STRUCTURE || (i.type==DROID && !isVTOL(i))).map(i=>i.cost))
    }

    enumStruct(me,"A0LasSatCommand").forEach(lassat=>{
        //computer player have no lassat cooldown, it implements by script
        if (gameTime-lassat.weapons[0].lastFired<300*1000)return

        //find all ground droid and structure of enemies
        var targetTeam=[]
        players.forEach(i=>{
            if (isSpectator(i))return
            if (allianceExistsBetween(me,i))return
            targetTeam.push(...enumDroid(i).filter(i=>!isVTOL(i)),...enumStruct(i))
        })
        if (targetTeam.length==0)return

        //attack largest enemy cluster
        var obj=max(targetTeam,costSum)
        var cost2=costSum(obj)
        if (cost2>500)
        {
            activateStructure(lassat,obj)
            chat(ALL_PLAYERS,"Laser Satellite Effect:"+cost2)
        }
    })
}

