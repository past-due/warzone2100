//#region DACTION
/**
 * Constants of droid action. Access by droid.action
 */
/** 0  not doing anything */
const DACTION_NONE=0
/** 1  moving to a location */
const DACTION_MOVE=1
/** 2  building a structure */
const DACTION_BUILD=2
/** 3  used to be building a foundation for a structure */
const DACTION_UNUSED3=3
/** 4  demolishing a structure */
const DACTION_DEMOLISH=4
/** 5  repairing a structure */
const DACTION_REPAIR=5
/** 6  attacking something */
const DACTION_ATTACK=6
/** 7  observing something */
const DACTION_OBSERVE=7
/** 8  attacking something visible by a sensor droid */
const DACTION_FIRESUPPORT=8
/** 9  refuse to do anything aggressive for a fixed time */
const DACTION_SULK=9
/**  , */
const DACTION_UNUSED_2=10
/** 11  move transporter offworld */
const DACTION_TRANSPORTOUT=11
/** 12  wait for timer to move reinforcements in */
const DACTION_TRANSPORTWAITTOFLYIN=12
/** 13  move transporter onworld */
const DACTION_TRANSPORTIN=13
/** 14  repairing a droid */
const DACTION_DROIDREPAIR=14
/** 15  restore resistance points of a structure */
const DACTION_RESTORE=15
/**  , */
const DACTION_UNUSED=16
/** 17  */
const DACTION_MOVEFIRE=17
/** 18  moving to a new building location */
const DACTION_MOVETOBUILD=18
/** 19  moving to a new demolition location */
const DACTION_MOVETODEMOLISH=19
/** 20  moving to a new repair location */
const DACTION_MOVETOREPAIR=20
/** 21  moving around while building */
const DACTION_BUILDWANDER=21
/** 22  used to be moving around while building the foundation */
const DACTION_UNUSED4=22
/** 23  moving to a target to attack */
const DACTION_MOVETOATTACK=23
/** 24  rotating to a target to attack */
const DACTION_ROTATETOATTACK=24
/** 25  moving to be able to see a target */
const DACTION_MOVETOOBSERVE=25
/** 26  waiting to be repaired by a facility */
const DACTION_WAITFORREPAIR=26
/** 27  move to repair facility repair point */
const DACTION_MOVETOREPAIRPOINT=27
/** 28  waiting to be repaired by a facility */
const DACTION_WAITDURINGREPAIR=28
/** 29  moving to a new location next to droid to be repaired */
const DACTION_MOVETODROIDREPAIR=29
/** 30  moving to a low resistance structure */
const DACTION_MOVETORESTORE=30
/**  , */
const DACTION_UNUSED2=31
/** 32  moving to a rearming pad - VTOLS */
const DACTION_MOVETOREARM=32
/** 33  waiting for rearm - VTOLS */
const DACTION_WAITFORREARM=33
/** 34  move to rearm point - VTOLS - this actually moves them onto the pad */
const DACTION_MOVETOREARMPOINT=34
/** 35  waiting during rearm process- VTOLS */
const DACTION_WAITDURINGREARM=35
/** 36  a VTOL droid doing attack runs */
const DACTION_VTOLATTACK=36
/** 37  a VTOL droid being told to get off a rearm pad */
const DACTION_CLEARREARMPAD=37
/** 38  used by scout/patrol order when returning to route */
const DACTION_RETURNTOPOS=38
/** 39  used by firesupport order when sensor retreats */
const DACTION_FIRESUPPORT_RETREAT=39
/** 41  circling while engaging */
const DACTION_CIRCLE=41
