#!/bin/sh
# GameState research edit test: loading a savegame after the stats or research data changed.
# A cold load replays the completed research with the data loaded then and adds the remainder the savegame holds
# (what its upgrade tables held beyond that replay, ex. upgrades set by scripts).
# So edits to research results or base stats take effect, upgrades set by scripts are kept,
# and with the same data the saved tables come back exactly.
#
# Run A plays cam:CAM_2A, which starts with the first campaign's research completed (including R-Wpn-MG-ROF01,
# -20% FirePause for machine guns), and saves at <save tick>. Its savegame is then loaded and saved again straight
# away (--gamestate-resave-on-load), and compared with a new game played with the same data to the same tick:
#   B  same data: the upgrades section is unchanged, and nothing is reported.
#   C  a test mod makes R-Wpn-MG-ROF01 -50%: the upgrade tables are those of a new game with that mod
#      (every player and stat, with MG1Mk1 firePause 250 rather than 400), only the upgrades section differs from B,
#      and the difference is reported. The game also plays on from the savegame to <end tick> with this mod.
#   D  a test mod makes MG1Mk1's base firePause 6 (600 ms, was 5): the tables are those of a new game with that mod
#      (MG1Mk1 firePause 480).
#   G  a test mod changes only MG1Mk1's flightSpeed, which no upgrade uses: the upgrades section is unchanged, and
#      nothing is reported.
#   E  run A2 plays as A with a test mod whose campaign script adds 7 to the player's Machinegun damage through
#      Upgrades[] at the start, and its savegame holds that in the remainder. Then the mod gets the research edit of C
#      as well (a savegame mounts the mods it was saved with, by name), and loading the savegame keeps the +7 on top
#      of the recomputed upgrades.
#   F  a test mod renames a research topic: the load is refused (the stat index differs), instead of putting the
#      saved research on the wrong topics.
#
# usage: researchedit.sh <warzone2100 binary> [<save tick> [<end tick>]]   (default 3000 6000)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds),
#              SEED (default 1, passed as --gamestate-test-seed).
# Exit status: 0 passed, 1 failed, 2 a run failed or the campaign data is no longer what this test expects.
set -u
BIN=${1:?binary}
SAVETICK=${2:-3000}
ENDTICK=${3:-6000}
HERE=$(cd "$(dirname "$0")" && pwd)
DATA="$HERE/../../data/base"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
CFG="$OUT/config"
SAVES="$CFG/savegames/campaign"
SAVE="researchedit_$SAVETICK"
REPORTED="differ from this savegame"
mkdir -p "$SAVES" "$CFG/mods/global"
for s in "$SAVE" "${SAVE}_B" "${SAVE}_C" "${SAVE}_D" "${SAVE}_G" "${SAVE}_F" "${SAVE}_newC" "${SAVE}_newD" "${SAVE}_script" "${SAVE}_script_C"; do
	rm -rf "$SAVES/$s" "$SAVES/$s.gam"
done

# The test mods: copies of campaign data files with one thing changed
python3 -I - "$DATA" "$CFG/mods/global" <<'EOF' || exit 2
import json, os, re, sys, zipfile
data, mods = sys.argv[1], sys.argv[2]
def load(path):
    with open(os.path.join(data, path), encoding='utf-8') as f:
        return json.load(f)
def mod(name, files):
    with zipfile.ZipFile(os.path.join(mods, name), 'w') as z:
        for path, content in files.items():
            z.writestr(path, content if isinstance(content, str) else json.dumps(content, indent=4))

research, weapons = load('stats/research.json'), load('stats/weapons.json')
rof = [r for r in research['R-Wpn-MG-ROF01']['results'] if r['parameter'] == 'FirePause']
with open(os.path.join(data, 'script/campaign/cam2-a.js'), encoding='utf-8') as f:
    cam2a = f.read()
if (len(rof) != 1 or rof[0]['value'] != -20 or weapons['MG1Mk1'].get('firePause') != 5
        or weapons['MG1Mk1'].get('name') != 'Machinegun' or 'R-Sys-SpyTurret' not in research
        or not re.search(r'^function eventStartLevel\(\)', cam2a, re.M)):
    print('researchedit: the campaign data is no longer what this test expects (R-Wpn-MG-ROF01 FirePause -20,'
          ' MG1Mk1 "Machinegun" firePause 5, R-Sys-SpyTurret, cam2-a.js eventStartLevel)')
    sys.exit(1)

rof[0]['value'] = -50
mod('researchedit_rof.wz', {'stats/research.json': research})
rof[0]['value'] = -20
w = json.loads(json.dumps(weapons))
w['MG1Mk1']['firePause'] = 6
mod('researchedit_base.wz', {'stats/weapons.json': w})
w = json.loads(json.dumps(weapons))
w['MG1Mk1']['flightSpeed'] = w['MG1Mk1'].get('flightSpeed', 0) + 100
mod('researchedit_unused.wz', {'stats/weapons.json': w})
renamed = {('R-A-RenamedSpyTurret' if k == 'R-Sys-SpyTurret' else k): v for k, v in research.items()}
renamed['R-A-RenamedSpyTurret']['id'] = 'R-A-RenamedSpyTurret'
mod('researchedit_rename.wz', {'stats/research.json': renamed})
script = {'script/campaign/cam2-a.js':
    re.sub(r'^function eventStartLevel\(\)', 'function __testOrigStartLevel()', cam2a, flags=re.M) + '''
// researchedit.sh: an upgrade set by the script
function eventStartLevel()
{
	__testOrigStartLevel();
	Upgrades[CAM_HUMAN_PLAYER].Weapon["Machinegun"].Damage += 7;
}
'''}
mod('researchedit_script.wz', script)
rof[0]['value'] = -50
script['stats/research.json'] = research
mod('researchedit_script_edited.wz', script)
EOF

COMMON="--configdir=$CFG --headless --autogame --gamestate-test-seed=$SEED"
# play <save> [<game options>]: play CAM_2A to the save tick and save there
play() {
	NAME=$1
	shift
	timeout "$RUN_TIMEOUT" "$BIN" $COMMON --game=CAM_2A "$@" --gamestate-save-at-tick="$SAVETICK:savegames/campaign/$NAME.gam" \
		--gamestate-exit-at-tick="$((SAVETICK + 100))" > "$OUT/$NAME.log" 2>&1
	[ -f "$SAVES/$NAME/gamestate.wz" ] || { echo "$SAVE: RUN $NAME FAILED (see $OUT/$NAME.log)"; exit 2; }
}
# resave <save> <as> [<game options>]: load the savegame and save it again as <as>
resave() {
	FROM=$1
	AS=$2
	shift 2
	timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadcampaign="$FROM" "$@" \
		--gamestate-resave-on-load="savegames/campaign/$AS.gam" > "$OUT/$AS.log" 2>&1
	[ -f "$SAVES/$AS/gamestate.wz" ] || { echo "$SAVE: RUN $AS FAILED (see $OUT/$AS.log)"; exit 2; }
}
play "$SAVE"
resave "$SAVE" "${SAVE}_B"
resave "$SAVE" "${SAVE}_C" --mod=researchedit_rof.wz
resave "$SAVE" "${SAVE}_D" --mod=researchedit_base.wz
resave "$SAVE" "${SAVE}_G" --mod=researchedit_unused.wz
play "${SAVE}_newC" --mod=researchedit_rof.wz
play "${SAVE}_newD" --mod=researchedit_base.wz
play "${SAVE}_script" --mod=researchedit_script.wz
cp "$CFG/mods/global/researchedit_script_edited.wz" "$CFG/mods/global/researchedit_script.wz"
resave "${SAVE}_script" "${SAVE}_script_C"
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadcampaign="$SAVE" --mod=researchedit_rof.wz \
	--gamestate-exit-at-tick="$ENDTICK" > "$OUT/${SAVE}_C_continue.log" 2>&1
CONTINUE=$?
# A refused load goes back to the main menu, so stop the game once it has failed (or saved)
timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadcampaign="$SAVE" --mod=researchedit_rename.wz \
	--gamestate-resave-on-load="savegames/campaign/${SAVE}_F.gam" > "$OUT/${SAVE}_F.log" 2>&1 &
PID=$!
while kill -0 $PID 2> /dev/null; do
	grep -q "Restarting main menu" "$OUT/${SAVE}_F.log" 2> /dev/null && break
	[ -f "$SAVES/${SAVE}_F/gamestate.wz" ] && sleep 1 && break
	sleep 1
done
kill $PID 2> /dev/null
wait $PID 2> /dev/null

FAILED=0
fail() { echo "$SAVE: FAILED: $1"; FAILED=1; }
grep -q "$REPORTED" "$OUT/${SAVE}_B.log" && fail "B: a difference was reported with the same data"
grep -q "$REPORTED" "$OUT/${SAVE}_C.log" || fail "C: the difference was not reported"
grep -q "$REPORTED" "$OUT/${SAVE}_D.log" || fail "D: the difference was not reported"
grep -q "$REPORTED" "$OUT/${SAVE}_G.log" && fail "G: a difference was reported for a stat no upgrade uses"
[ "$CONTINUE" -eq 0 ] || fail "C: playing on to $ENDTICK exited with status $CONTINUE (see $OUT/${SAVE}_C_continue.log)"
[ -f "$SAVES/${SAVE}_F/gamestate.wz" ] && fail "F: the savegame was loaded with a research topic renamed"
grep -q "the research topics are not the ones" "$OUT/${SAVE}_F.log" || fail "F: the renamed topic was not reported (see $OUT/${SAVE}_F.log)"
python3 -I - "$SAVES" "$SAVE" <<'EOF' || FAILED=1
import json, os, sys, zipfile
saves, save = sys.argv[1], sys.argv[2]

def load(name):
    path = os.path.join(saves, name, 'gamestate.wz')
    with zipfile.ZipFile(path) as z:
        return json.loads(z.read('gamestate.json'))['gameState']

def value(doc, table, stat, player, field):
    for entry in doc['upgrades']['tables'][table]:
        if entry['player'] == player and entry['id'] == stat:
            return entry.get(field)
    return None

failures = []
def check(cond, msg):
    if not cond:
        failures.append(msg)

a, b, c, d, g = (load(save + s) for s in ('', '_B', '_C', '_D', '_G'))
new_c, new_d, script, script_c = (load(save + s) for s in ('_newC', '_newD', '_script', '_script_C'))
check(value(a, 'Weapon', 'MG1Mk1', 0, 'firePause') == 400, 'A: MG1Mk1 firePause is %s, expected 400 (500 - 20%%)' % value(a, 'Weapon', 'MG1Mk1', 0, 'firePause'))
check(a['upgrades'].get('remainder') == {'tables': {}, 'players': []},
      'A: the remainder is not empty (the replay of the research differs from the game): %s' % json.dumps(a['upgrades'].get('remainder'))[:300])
check(b['upgrades'] == a['upgrades'], 'B: the upgrades section changed with the same data')
check(g['upgrades'] == a['upgrades'], 'G: the upgrades section changed for a stat no upgrade uses')
for name, loaded, new in (('C', c, new_c), ('D', d, new_d)):
    check(loaded['upgrades']['tables'] == new['upgrades']['tables'],
          '%s: the upgrade tables differ from those of a new game with the same mod' % name)
    check(loaded['upgrades']['players'] == new['upgrades']['players'],
          '%s: the research bookkeeping differs from that of a new game with the same mod' % name)
check(value(c, 'Weapon', 'MG1Mk1', 0, 'firePause') == 250, 'C: MG1Mk1 firePause is %s, expected 250 (500 - 50%%)' % value(c, 'Weapon', 'MG1Mk1', 0, 'firePause'))
check(value(d, 'Weapon', 'MG1Mk1', 0, 'firePause') == 480, 'D: MG1Mk1 firePause is %s, expected 480 (600 - 20%%)' % value(d, 'Weapon', 'MG1Mk1', 0, 'firePause'))
changed = sorted(k for k in set(b) | set(c) if k != 'upgrades' and b.get(k) != c.get(k))
check(not changed, 'C: sections other than upgrades differ from B: %s' % ', '.join(changed))
damage = value(a, 'Weapon', 'MG1Mk1', 0, 'damage')
check(value(script, 'Weapon', 'MG1Mk1', 0, 'damage') == damage + 7,
      'E: MG1Mk1 damage is %s with the script, expected %s + 7' % (value(script, 'Weapon', 'MG1Mk1', 0, 'damage'), damage))
check(any(e['id'] == 'MG1Mk1' and e['player'] == 0 and e.get('damage') == 7 for e in script['upgrades'].get('remainder', {}).get('tables', {}).get('Weapon', [])),
      'E: the remainder does not hold the +7 damage set by the script')
check(value(script_c, 'Weapon', 'MG1Mk1', 0, 'damage') == damage + 7,
      'E: with the research edit, MG1Mk1 damage is %s, expected %s + 7 (the upgrade set by the script was lost)' % (value(script_c, 'Weapon', 'MG1Mk1', 0, 'damage'), damage))
check(value(script_c, 'Weapon', 'MG1Mk1', 0, 'firePause') == 250, 'E: with the research edit, MG1Mk1 firePause is %s, expected 250' % value(script_c, 'Weapon', 'MG1Mk1', 0, 'firePause'))
for f in failures:
    print('%s: FAILED: %s' % (save, f))
sys.exit(1 if failures else 0)
EOF
[ "$FAILED" -eq 0 ] && echo "$SAVE: passed (B same data, G unused stat: unchanged; C research edit, D base stat edit: as a new game with the mod; E script upgrade kept; F renamed topic refused; played on to $ENDTICK)"
exit "$FAILED"
