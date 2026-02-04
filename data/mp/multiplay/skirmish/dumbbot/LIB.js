//#region random
/**
 * Return random integer in range [a, b], including both end points.
 * @param {Number} a 
 * @param {Number} b 
 */
function randint(a, b) {
    return Math.floor(a + Math.random() * (b - a + 1))
}

/**
 * @template T
 * @param {T[]} arr
 * @returns {T}
 */
function choose(arr = []) {
    return arr[randint(0, arr.length - 1)]
}





//#region collection T[]->T[]
/**
 * @template T
 * @param {T[]} arr
 * @param {(a:T)=>String | Number} key
 * @returns {T[]}
 */
function unique(arr=[],key=i=>JSON.stringify(i))
{
    return [...new Map(arr.map(i=>[key(i),i])).values()]
}


function dropna(arr=[])
{
    return arr.filter(i=>!isNaN(i))
}

/**
 * @template T
 * @param {T[]} arr
 * @param {(a:T)=>Number} key
 * @returns {T[][]}
 */
function groupBy(arr=[],key=i=>Number(i))
{
    var group=[]
    arr.forEach((i)=>{
        var k=key(i)
        if (group[k]===undefined)group[k]=[]
        group[k].push(i)
    })
    return group
}






//#region aggregation  T[]->T

/** return index of minimum
 * @template T
 * @param {T[]} arr
 * @param {(x:T)=>Number} key
 * @returns {Number}
 * */
function argmin(arr=[],key=i=>Number(i))
{
    var k=arr.map(x=>key(x))
    var kmin=Math.min(...dropna(k))
    return k.findIndex(x=>x==kmin)
}
/**
 * @template T
 * @param {T[]} arr
 * @param {(x:T)=>Number} key
 * @returns {T}
 */
function min(arr,key=i=>Number(i))
{
    return arr[argmin(arr,key)]
}
/**
 * @template T
 * @param {T[]} arr
 * @param {(x:T)=>Number} key
 * @returns {T}
 */
function max(arr=[],key=i=>Number(i))
{
    return min(arr,i=>-key(i))
}

/**
 * @param {Number[]} arr
 */
function sum(arr=[])
{
    arr=dropna(arr.map(i=>Number(i)))
    return arr.reduce((a,b)=>(a+b),0)
}

/**
 * @param {Number[]} arr
 */
function mean(arr=[])
{
    arr=dropna(arr.map(i=>Number(i)))
    return sum(arr) / arr.length
}










//#region mapping  T1[]->T2[]

/** return index，from minimum's to maximum's
 * @template T
 * @param {T[]} arr
 * @param {(x:T)=>Number} key
 * @returns {Number[]}
 * */
function argsort(arr=[],key=i=>Number(i))
{
    var k=arr.map((x,i)=>key(x)).map(x=>isNaN(x)?Infinity:x).map((x,i)=>[x,i])
    k.sort((a,b)=>(a[0]-b[0]))
    return k.map(a=>a[1])
}

/** return index in sorted array
 * @template T
 * @param {T[]} arr
 * @param {(x:T)=>Number} key
 * @returns {Number[]}
 * */
function argmap(arr=[],key=i=>Number(i))
{
    var arr2=argsort(arr,key)
    var arr3=new Array(arr.length)
    arr2.forEach((i,j)=>arr3[i]=j)
    return arr3
}











//#region vector
function distBetween(object,object2)
{
    if (object===undefined || object2===undefined)return Infinity
    return Math.hypot(object.x-object2.x,object.y-object2.y)
}


function distTo(object)
{
    return (object2)=>distBetween(object,object2)
}

/**
 * 
 * @param {_pos} src 
 * @param {_pos} dst 
 * @param {Number} yaw 
 * @param {Number} dist 
 * @returns {_pos}
 */
function yawTo(src,dst,yaw=0,dist=5)
{
    var [dx,dy]=[dst.x-src.x, dst.y-src.y]
    var norm=Math.hypot(dx,dy)
    var [dx,dy]=[dx/norm, dy/norm]
    var [nx,ny]=[-dy,dx]
    return {
        "type":POSITION, 
        "x":Math.round(src.x+dx*dist+nx*yaw),
        "y":Math.round(src.y+dy*dist+ny*yaw),
    }
}

/**
 * 
 * @param {_pos[]} posArr 
 * @param {Number[]} weightArr 
 * @returns {_pos}
 */
function mix(posArr,weightArr)
{
    var [x,y]=[0,0]

    for (var i=0;i<posArr.length;i++)
    {
        x+=posArr[i].x*weightArr[i]
        y+=posArr[i].y*weightArr[i]       
    }

    return {
        "type":POSITION,
        "x":x,
        "y":y
    }
}




/**
 * @template T
 * @param {(a:T)=>Number} func
 * @returns {(a:T,b:T)=>Number}
 */
function by(func)
{
    return (a,b)=>(func(a)-func(b))
}

/**
 * Remove a element from array
 * @template T
 * @param {T[]} arr
 * @param {T} x
 */
function remove(arr,x)
{
    arr.splice(arr.findIndex(i=>i==x),1)
}

/**
 * Moveable, but invalid for order or structure destination
 * @param {Number} x 
 * @param {Number} y 
 * @returns 
 */
function isEdge(x,y)
{
    const rect=getScrollLimits()
    return x<rect.x+4 || x>rect.x2-4 || y<rect.y+4 || y>rect.y2-4
}

/**
 * @param {_droid} droid 
 */
function truckBusy(droid)
{
    return (
        droid.action==DACTION_MOVETOBUILD ||
        droid.action==DACTION_BUILD ||
        droid.action==DACTION_MOVETODEMOLISH ||
        droid.action==DACTION_DEMOLISH ||
        droid.action==DACTION_BUILDWANDER
    )
}

function weaponRange(player)
{
    var nRange=0
    var nCount=1
    for (const arr of [enumArmy(player).filter(i=>!isVTOL(i)), enumStruct(player).filter(i=>i.weapons.length>0)])
    {
        for (const i of arr)
        {
            nRange+=objectWeaponStat(i).MaxRange
            nCount+=1
        }
    }
    return nRange/128/nCount
}










/**
 * @param {_droid | _struct} object 
 * @returns {_weaponStat}
 */
function objectWeaponStat(object)
{
    return Stats.Weapon[object.weapons[0].fullname]
}

