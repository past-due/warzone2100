/**
 * Query component Stats|Upgrades of Droid|Structure.
 * (Code shorter and have better type hint)
 */

//#region vscode 1.96.2+

/** Map id to name */
const _idName = {}
// @ts-ignore
for (const [category, categoryData] of Object.entries(Stats)) {
    try {
        for (const [item, itemData] of Object.entries(categoryData)) {
            _idName[itemData.Id] = item
        }
    }
    catch { }
}

/**
 * convert `Id` (used at droid property & buildDroid argument) to `Name` (used at Stats)
 * @param {String} Id 
 * @returns {String}
 */
function getComponentName(Id) {
    return _idName[Id]
}

/**
 * More research Stats (Requires, ResultComponents, ResultStructures)
 * @param {String} Id 
 * @returns {typeof Stats.Research["Needle Gun"]}
 */
function getResearchStats(Id) {
    return Stats.Research[getResearch(Id, me).name]
}



class _asStats_droid {
    /**
     * @param {_droid} object 
     */
    constructor(object) {
        this.object = object
    }

    /**
     * replace objectWeaponStat
     * @returns {typeof Stats.Weapon.Lancer}
     */
    get weapon() {
        return Stats.Weapon[this.object.weapons[0].fullname]
    }

    /**
     * @returns {(typeof Stats.Weapon.Lancer)[]}
     */
    get weapons() {
        return this.object.weapons.map(i => Stats.Weapon[i.fullname])
    }

    /**
     * @returns {typeof Stats.Propulsion.Wheels}
     */
    get prop() {
        return Stats.Propulsion[getComponentName(this.object.propulsion)]
    }

    /**
     * @returns {typeof Stats.Body.Cobra}
     */
    get body() {
        return Stats.Body[getComponentName(this.object.body)]
    }

    /**
     * @returns {typeof Stats.Sensor["*Default Sensor*"]}
     */
    get sensor() {
        const x = Stats.Sensor
        if (this.object.droidType == DROID_SENSOR) {
            if (this.object.cost >= Stats.Sensor["Wide Spectrum Sensor"].BuildPower)
                return x["Wide Spectrum Sensor"]
            return x["Sensor Turret"]
        }
        return x["*Default Sensor*"]
    }

    /**
     * @returns {typeof Stats.Construct.Truck}
     */
    get construct() {
        if (this.object.propulsion == "CyborgLegs") {
            return Stats.Construct["*Combat Engineer*"]
        }
        return Stats.Construct.Truck
    }
}

class _asStats_structure {
    /**
     * @param {_struct} object 
     */
    constructor(object) {
        this.object = object
    }

    /**
     * replace objectWeaponStat
     * @returns {typeof Stats.Weapon.Lancer}
     */
    get weapon() {
        return Stats.Weapon[this.object.weapons[0].fullname]
    }

    /**
     * @returns {typeof Stats.Building["Cannon Fortress"]}
     */
    get building() {
        return Stats.Building[this.object.name]
    }

    /**
     * @returns {typeof Stats.Sensor["*Default Sensor*"]}
     */
    get sensor() {
        const x = Stats.Sensor
        switch (this.object.name) {
            case "Hardened Sensor Tower":
                return x["Hardened Sensor Tower"]
            case "Wide Spectrum Sensor Tower":
                return x["Wide Spectrum Sensor"]
            case "Satellite Uplink Center":
                return x["Uplink Sensor"]
            case "Sensor Tower":
                return x["*Tower Sensor*"]
            default:
                if (Stats.Building[this.object.name].Id.startsWith("X-Super-"))
                    return x["*Fortress Sensor*"]
                return x["*Default Sensor*"]
        }
    }
}





class _asUpgrades_droid {
    /**
     * @param {_droid} object 
     */
    constructor(object) {
        this.object = object
    }

    /**
     * replace objectWeaponStat
     * @returns {typeof Upgrades[0]["Weapon"]["Lancer"]}
     */
    get weapon() {
        return Upgrades[this.object.player].Weapon[this.object.weapons[0].fullname]
    }

    /**
     * @returns {(typeof Upgrades[0]["Weapon"]["Lancer"])[]}
     */
    get weapons() {
        return this.object.weapons.map(i => Upgrades[this.object.player].Weapon[i.fullname])
    }

    /**
     * @returns {typeof Upgrades[0]["Propulsion"]["Wheels"]}
     */
    get propulsion() {
        return Upgrades[this.object.player].Propulsion[getComponentName(this.object.propulsion)]
    }

    /**
     * @returns {typeof Upgrades[0]["Body"]["Cobra"]}
     */
    get body() {
        return Upgrades[this.object.player].Body[getComponentName(this.object.body)]
    }

    /**
     * @returns {typeof Upgrades[0]["Sensor"]["*Default Sensor*"]}
     */
    get sensor() {
        const x = Upgrades[this.object.player].Sensor
        if (this.object.droidType == DROID_SENSOR) {
            if (this.object.cost >= Stats.Sensor["Wide Spectrum Sensor"].BuildPower)
                return x["Wide Spectrum Sensor"]
            return x["Sensor Turret"]
        }
        return x["*Default Sensor*"]
    }

    /**
     * @returns {typeof Upgrades[0]["Construct"]["Truck"]}
     */
    get construct() {
        if (this.object.propulsion == "CyborgLegs") {
            return Upgrades[this.object.player].Construct["*Combat Engineer*"]
        }
        return Upgrades[this.object.player].Construct.Truck
    }
}

class _asUpgrades_structure {
    /**
     * @param {_struct} object 
     */
    constructor(object) {
        this.object = object
    }

    /**
     * @returns {typeof Upgrades[0]["Weapon"]["Lancer"]}
     */
    get weapon() {
        return Upgrades[this.object.player].Weapon[this.object.weapons[0].fullname]
    }

    /**
     * @returns {typeof Upgrades[0]["Building"]["Cannon Fortress"]}
     */
    get building() {
        return Upgrades[this.object.player].Building[this.object.name]
    }

    /**
     * @returns {typeof Upgrades[0]["Sensor"]["*Default Sensor*"]}
     */
    get sensor() {
        const x = Upgrades[this.object.player].Sensor
        switch (this.object.name) {
            case "Hardened Sensor Tower":
                return x["Hardened Sensor Tower"]
            case "Wide Spectrum Sensor Tower":
                return x["Wide Spectrum Sensor"]
            case "Satellite Uplink Center":
                return x["Uplink Sensor"]
            case "Sensor Tower":
                return x["*Tower Sensor*"]
            default:
                if (Stats.Building[this.object.name].Id.startsWith("X-Super-"))
                    return x["*Fortress Sensor*"]
                return x["*Default Sensor*"]
        }
    }
}








/**
 * `asStats(object).weapon` for `Stats.Weapon[object.weapons[0].fullname]` etc.
 * @param {_droid | _struct} object 
 * @returns {_asStats_droid & _asStats_structure}
 */
function asStats(object) {
    // @ts-ignore
    if (object.type == STRUCTURE) return new _asStats_structure(object)
    // @ts-ignore
    return new _asStats_droid(object)
}


/**
 * `asUpgrades(object).weapon` for `Upgrades[object.player].Weapon[object.weapons[0].fullname]` etc.
 * @param {_droid | _struct} object 
 * @returns {_asUpgrades_droid & _asUpgrades_structure}
 */
function asUpgrades(object) {
    // @ts-ignore
    if (object.type == STRUCTURE) return new _asUpgrades_structure(object)
    // @ts-ignore
    return new _asUpgrades_droid(object)
}

(() => {
    dumbug(enumStruct(me).map(i => asStats(i).building.Id))
    dumbug(enumStruct(me).map(i => asStats(i).building.HitPoints))
    dumbug(enumDroid(me).map(i => asStats(i).body.Power))
    dumbug(enumDroid(me).map(i => asStats(i).weapons.map(j => j.Damage)))

    dumbug(enumStruct(me).map(i => asUpgrades(i).building.Limit))
    dumbug(enumStruct(me).map(i => asUpgrades(i).building.HitPoints))
    dumbug(enumDroid(me).map(i => asUpgrades(i).body.Power))
    dumbug(enumDroid(me).map(i => asUpgrades(i).weapons.map(j => j.Damage)))
})