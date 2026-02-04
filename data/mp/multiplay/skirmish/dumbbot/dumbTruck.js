const _coreMod={
    "A0ResearchModule1":[["A0ResearchFacility",1]],
    "A0PowMod1":[["A0PowerGenerator",1]],
    "A0FacMod1":[["A0LightFactory",2],["A0VTolFactory1",2]],
}




/**
 * 
 * @param {String} struct 
 * @param {Number} limit 
 * @returns 
 */
function structureTooMuch(struct,limit)
{
    var isMod=struct in _coreMod
    var isOil=struct == "A0ResourceExtractor"
    if (isMod)
    {
        var modLimit=0
        var modCount=0
        for (var [baseStruct, baseLimit] of _coreMod[struct])
        {
            modLimit+=countStruct(baseStruct,me) * baseLimit
            modCount+=enumStruct(me,baseStruct).map(i=>i.modules).reduce((a,b)=>a+b,0)
        }
        var tooMuch=modCount>=Math.min(getStructureLimit(struct,me),limit,modLimit)
    }
    else if (isOil)
    {
        var tooMuch=countStruct(struct,me)>=Math.min(getStructureLimit("A0PowerGenerator",me)*4,limit)
    }
    else
    {
        var tooMuch=countStruct(struct,me)>=Math.min(getStructureLimit(struct,me),limit)
    }
    return tooMuch
}




/**
 * 
 * @param {String} struct 
 * @param {_pos} pos
 * @param {_droid[]} truckTeam 
 * @returns {_pos[]}
 */
function structurePositions(struct,pos,truckTeam)
{
    var posTeam=[]
    var isMod=struct in _coreMod
    var isOil=struct == "A0ResourceExtractor"
    if (isMod)
    {
        for (var [baseStruct, baseLimit] of _coreMod[struct])
        {
            posTeam.push(...enumStruct(me,baseStruct).filter(i=>i.modules<baseLimit))
        }
    }
    else if (isOil)
    {
        posTeam.push(...enumFeature(ALL_PLAYERS,"OilResource").filter(i=>propulsionCanReach("wheeled01", home.x,home.y, i.x,i.y)))
    }
    else
    {
        posTeam.push(...truckTeam.map(i=>pickStructLocation(i,struct,pos.x,pos.y)))
        posTeam.push(...truckTeam.map(i=>pickStructLocation(i,struct,i.x,i.y)))
    }
    return posTeam
}




/**
 * @param {_pos} pos 
 * @param {String} struct 
 * @param {_droid[]} truckTeam 
 */
function buildAt(pos,struct,truckTeam,limit=Infinity)
{
    if (pos===undefined)return true
    if (struct===undefined)return true
    if (truckTeam.length==0)return true
    var isMod=struct in _coreMod
    var isOil=struct == "A0ResourceExtractor"
    var tooMuch=structureTooMuch(struct,limit)
    
    //Not researched
    if (!isStructureAvailable(struct,me) && !tooMuch)return true
    //Being built
    var beingBuilt=enumStruct(me,struct).filter(i=>i.status==BEING_BUILT)
    if (beingBuilt.length>0)
    {
        truckTeam.forEach(i=>{
            var bestTarget=min(beingBuilt,distTo(i))
            orderDroidObj(i,DORDER_HELPBUILD,bestTarget)
        })
        return false
    }
    //Too much
    if (tooMuch)return true
    
    var posTeam=structurePositions(struct,pos,truckTeam)
    //No space
    if (posTeam.length==0)return true

    var bestTruck=min(truckTeam,distTo(home))
    if (isMod || isOil) var bestPos=min(posTeam,i=>distBetween(i,bestTruck))
    else if (struct=="A0RepairCentre3" || struct=="A0VTolFactory1") var bestPos=min(posTeam,i=>distBetween(i,pos))
    else var bestPos=min(posTeam,i=>distBetween(i,bestTruck) + .5*distBetween(i,pos))

    //Need to build
    bestTruck=min(truckTeam,distTo(bestPos))
    truckTeam.forEach(i=>{
        return orderDroidBuild(i,DORDER_BUILD,struct,bestPos.x,bestPos.y)
    })
    return false
}

function haveOil(x=0,y=0)
{
    var a=enumArea(x,y,x+1,y+1,ALL_PLAYERS,false)
    var b=a.filter(i=>i.type==STRUCTURE)
    return (b.length>0)
}

/**
 * @param {_droid[]} truckTeam 
 */
function buildOil(truckTeam)
{
    truckTeam.sort(by(distTo(home)))
    truckTeam.reverse()

    var toBuild=derrickPositions.filter(i=>!haveOil(i.x,i.y))
    //nearest to truck
    for (var i of truckTeam)
    {
        if (toBuild.length==0)return
        var j=min(toBuild,j=>distBetween(i,j)-0.5*distBetween(home,j))
        orderDroidBuild(i,DORDER_BUILD,"A0ResourceExtractor",j.x,j.y)
        remove(toBuild,j)
    }
}




function dumbTruck()
{
    function countStructDone(name)
    {
        return enumStruct(me,name).filter(i=>i.status==BUILT).length
    }

    var truckTeam=enumDroid(me,DROID_CONSTRUCT)
    truckTeam.sort(by(i=>i.born-i.id&0xff))

    if (truckTeam.length==0)return
    var nPow=countStructDone("A0PowerGenerator")
    var nOil=countStructDone("A0ResourceExtractor")
    var nDerr=derrickPositions.filter(i=>!haveOil(i.x,i.y)).filter(i=>droidCanReach(truckTeam[0],i.x,i.y)).length

    var nOilEffect=Math.min(nOil,nPow*4)

    var nFact=countStructDone("A0LightFactory")
    var nRes=countStructDone("A0ResearchFacility")+enumResearch().length
    var nVtol=enumDroid(me,DROID_WEAPON).filter(i=>isVTOL(i)).length
    var done=true


    var nOilTruck=0
    if (nFact>=2)
    {
        nOilTruck=Math.round(truckTeam.length*.5)
        if (nDerr>10 && nOil<40)
        {
            nOilTruck=Math.round(truckTeam.length*.3)
        }
    }


    nOilTruck=Math.min(nDerr,nOilTruck)
    var truckTeam2=truckTeam.splice(0,nOilTruck).filter(i=>!truckBusy(i))
    buildOil(truckTeam2)

    truckTeam=truckTeam.filter(i=>!truckBusy(i))

    if (done)done=buildAt(home,"A0LightFactory",truckTeam,Math.floor(2+nOilEffect/8))
    if (done)done=buildAt(home,"A0ResearchFacility",truckTeam,nRes)
    if (done)done=buildAt(home,"A0PowerGenerator",truckTeam,nOil/4)
    if (done)done=buildAt(home,"A0CyborgFactory",truckTeam,nOilEffect/6)

    if (done && playerPower(me)>400)done=buildAt(home,"A0ResearchModule1",truckTeam,5)
    if (done)done=buildAt(home,"A0PowMod1",truckTeam)

    if (done)done=buildAt(home,"A0CommandCentre",truckTeam,2)
    if (done)done=buildAt(home,"A0FacMod1",truckTeam,2)
    if (done)done=buildAt(home,"A0LasSatCommand",truckTeam)
    if (done)done=buildAt(home,"A0VTolFactory1",truckTeam)
    if (done)done=buildAt(home,"A0VtolPad",truckTeam,Math.min(nVtol,5))
    if (done)done=buildAt(home,"A0FacMod1",truckTeam,6)

    //if (done)done=buildAt(home,"A0ComDroidControl",truckTeam,1)

    if (done)done=buildAt(home,"Sys-SensoTowerWS",truckTeam,1)
    if (done)done=buildAt(home,"Sys-CB-Tower01",truckTeam,1)
    if (done)done=buildAt(home,"Emplacement-Howitzer-Incendiary",truckTeam,5)
    if (done)done=buildAt(home,"Emplacement-Howitzer150",truckTeam,10)

    if (done)done=buildAt(home,"A0FacMod1",truckTeam)
    if (done)done=buildAt(attackPath[Math.floor(attackPath.length*.12)],"A0RepairCentre3",truckTeam)
    if (done)done=buildAt(attackPath[Math.floor(attackPath.length*.12)],"X-Super-MassDriver",truckTeam,1)

    if (done)done=buildAt(home,"A0Sat-linkCentre",truckTeam)
    if (done)done=buildAt(home,"A0VtolPad",truckTeam,nVtol)
    if (done)done=buildAt({type:POSITION, x:(home.x+pos.x)/2,y:(home.y+pos.y)/2},["Emplacement-HvART-pit","Emplacement-Rocket06-IDF","Emplacement-Howitzer150"].find(i=>isStructureAvailable(i,me)),truckTeam,100)
    
    structurePend=!done

    if (done && truckTeam.length>0) {
        const oilDrum=enumFeature(me,"OilDrum").filter(i=>safeDest(me,i.x,i.y))
        if (oilDrum.length==0)return
        const anchor=truckTeam[0]
        orderDroidObj(anchor,DORDER_RECOVER,min(oilDrum,distTo(anchor)))
    }
}
