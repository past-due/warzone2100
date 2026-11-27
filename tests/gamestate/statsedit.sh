#!/bin/sh
# GameState stats edit test: loading a savegame after research topics, components or structures were added or removed.
# Those are stored by ID, so entries added since start as in a new game, and those removed are dropped.
#
# A campaign game (CAM_2A) is saved at <save tick>, then loaded and saved again:
#   B  with a test mod that adds a research topic, a weapon and a structure, each first in its file
#      (so every other entry moves). The savegame loads, every saved entry keeps its state,
#      the new topic isn't researched, and the additions are reported.
#   C  with a test mod that removes a research topic, a weapon and a structure the game doesn't use.
#      The savegame loads, every other entry keeps its state, and the removals are reported.
#
# usage: statsedit.sh <warzone2100 binary> [save tick] (default 3000)
# Environment: OUT (default /tmp/gamestate-forkcrc), RUN_TIMEOUT (default 600 seconds), SEED (default 1).
# Exit status: 0 passed, 1 a check failed, 2 a run failed.

BIN=${1:?usage: statsedit.sh <warzone2100 binary> [save tick]}
SAVETICK=${2:-3000}
HERE=$(cd "$(dirname "$0")" && pwd)
DATA="$HERE/../../data/base"
OUT=${OUT:-/tmp/gamestate-forkcrc}
RUN_TIMEOUT=${RUN_TIMEOUT:-600}
SEED=${SEED:-1}
CFG="$OUT/config"
SAVES="$CFG/savegames/campaign"
SAVE="statsedit_$SAVETICK"
COMMON="--configdir=$CFG --headless --autogame --gamestate-test-seed=$SEED"
mkdir -p "$SAVES" "$CFG/mods/global"
for s in "$SAVE" "${SAVE}_B" "${SAVE}_C"; do
	rm -rf "$SAVES/$s" "$SAVES/$s.gam"
done

timeout "$RUN_TIMEOUT" "$BIN" $COMMON --game=CAM_2A --gamestate-save-at-tick="$SAVETICK:savegames/campaign/$SAVE.gam" \
	--gamestate-exit-at-tick="$((SAVETICK + 100))" > "$OUT/$SAVE.log" 2>&1
[ -f "$SAVES/$SAVE/gamestate.wz" ] || { echo "$SAVE: RUN A FAILED (see $OUT/$SAVE.log)"; exit 2; }

# The test mods: copies of campaign data files with entries added or removed
# (only what nothing refers to and the saved game doesn't use).
python3 -I - "$DATA" "$CFG/mods/global" "$SAVES/$SAVE/gamestate.wz" <<'EOF' || exit 2
import json, os, sys, zipfile
data, mods, saved = sys.argv[1], sys.argv[2], sys.argv[3]
def load(path):
    with open(os.path.join(data, path), encoding='utf-8') as f:
        return json.load(f)
def mod(name, files):
    with zipfile.ZipFile(os.path.join(mods, name), 'w') as z:
        for path, content in files.items():
            z.writestr(path, json.dumps(content, indent=4))
def first(d, key, value):
    return {key: value, **d}
with zipfile.ZipFile(saved) as z:
    doc = json.loads(z.read('gamestate.json'))['gameState']

research, weapons, structures = load('stats/research.json'), load('stats/weapons.json'), load('stats/structure.json')
topic = json.loads(json.dumps(research['R-Wpn-MG-ROF01']))
topic.update({'id': 'R-Test-Added', 'name': 'Test Added Topic', 'requiredResearch': ['R-Wpn-MG1Mk1'], 'results': []})
weapon = dict(weapons['MG1Mk1'], id='TestAddedWeapon', name='Test Added Weapon')
structure = dict(structures['A0HardcreteMk1Wall'], id='TestAddedStructure', name='Test Added Structure')
mod('statsedit_added.wz', {'stats/research.json': first(research, 'R-Test-Added', topic),
                           'stats/weapons.json': first(weapons, 'TestAddedWeapon', weapon),
                           'stats/structure.json': first(structures, 'TestAddedStructure', structure)})

used = {o['statId'] for w in (doc['world'], doc['mission']['world']) for o in w['structures']}
assert 'A0BaBaBunker' not in used, 'A0BaBaBunker is used by the saved game'
for d, key in ((research, 'R-Wpn-TUTMG'), (weapons, 'NX-CyborgChaingun'), (structures, 'A0BaBaBunker')):
    assert key in d, key + ' is not in the data any more'
    del d[key]
mod('statsedit_removed.wz', {'stats/research.json': research, 'stats/weapons.json': weapons, 'stats/structure.json': structures})
EOF

# A savegame that doesn't load goes back to the main menu and waits there, so stop it then
for r in B C; do
	[ "$r" = B ] && m=statsedit_added.wz || m=statsedit_removed.wz
	timeout "$RUN_TIMEOUT" "$BIN" $COMMON --loadcampaign="$SAVE" --mod="$m" \
		--gamestate-resave-on-load="savegames/campaign/${SAVE}_$r.gam" > "$OUT/${SAVE}_$r.log" 2>&1 &
	pid=$!
	while kill -0 "$pid" 2> /dev/null; do
		grep -q "Restarting main menu" "$OUT/${SAVE}_$r.log" 2> /dev/null && kill "$pid"
		sleep 1
	done
	wait "$pid"
	[ -f "$SAVES/${SAVE}_$r/gamestate.wz" ] || { echo "$SAVE: $r: the savegame didn't load (see $OUT/${SAVE}_$r.log)"; exit 1; }
done

FAILED=0
fail() { echo "$SAVE: $*"; FAILED=1; }
reported() { grep -q "Savegame $2 matched by ID: $3 no longer exist and were dropped, $4 added since" "$OUT/${SAVE}_$1.log"; }
reported B "research topics" 0 1 || fail "B: the added research topic was not reported"
reported B components 0 1 || fail "B: the added weapon was not reported"
reported B structures 0 1 || fail "B: the added structure was not reported"
reported C "research topics" 1 0 || fail "C: the removed research topic was not reported"
reported C components 1 0 || fail "C: the removed weapon was not reported"
reported C structures 1 0 || fail "C: the removed structure was not reported"

python3 -I - "$SAVES" "$SAVE" <<'EOF' || FAILED=1
import json, os, sys, zipfile
saves, save = sys.argv[1], sys.argv[2]
def load(name):
    with zipfile.ZipFile(os.path.join(saves, name, 'gamestate.wz')) as z:
        return json.loads(z.read('gamestate.json'))['gameState']
failures = []
def check(cond, msg):
    if not cond:
        failures.append(msg)

def byId(entries):
    return {e['id']: e for e in entries}
def sections(doc):
    return {'research topic': byId(doc['research']['topics']),
            'component': {e['id']: e for c in doc['availability']['comp'] for e in c},
            'structure': byId(doc['availability']['structType']),
            'structure limit': byId(doc['limits']['structLimits'])}

a, b, c = (load(save + s) for s in ('', '_B', '_C'))
sa, sb, sc = sections(a), sections(b), sections(c)
added = {'research topic': 'R-Test-Added', 'component': 'TestAddedWeapon', 'structure': 'TestAddedStructure', 'structure limit': 'TestAddedStructure'}
removed = {'research topic': 'R-Wpn-TUTMG', 'component': 'NX-CyborgChaingun', 'structure': 'A0BaBaBunker', 'structure limit': 'A0BaBaBunker'}
for what in sa:
    check(set(sb[what]) == set(sa[what]) | {added[what]}, 'B: the %ss are not those saved plus %s' % (what, added[what]))
    check(set(sc[what]) == set(sa[what]) - {removed[what]}, 'C: the %ss are not those saved minus %s' % (what, removed[what]))
    for id, entry in sa[what].items():
        check(id not in sb[what] or sb[what][id] == entry, 'B: %s %s changed' % (what, id))
        check(id not in sc[what] or sc[what][id] == entry, 'C: %s %s changed' % (what, id))
    check(removed[what] in sa[what], 'A: no %s %s' % (what, removed[what]))
new = sb['research topic'].get('R-Test-Added', {})
check(new.get('status') == [0] * len(new.get('status', [])) and new.get('points') == [0] * len(new.get('points', [])),
      'B: the added research topic is not new: %s' % json.dumps(new))
mg = sa['research topic']['R-Wpn-MG1Mk1']
check(mg['status'][0] != 0, 'A: R-Wpn-MG1Mk1, the prerequisite of the added topic, is not researched by player 0')
for doc, run in ((b, 'B'), (c, 'C')):
    check(len(doc['world']['structures']) == len(a['world']['structures']) and len(doc['world']['droids']) == len(a['world']['droids']),
          '%s: the world objects changed' % run)

for f in failures:
    print('%s: %s' % (save, f))
sys.exit(1 if failures else 0)
EOF

[ "$FAILED" -eq 0 ] && echo "$SAVE: passed (B topic, weapon and structure added, C removed; saved at $SAVETICK)"
exit "$FAILED"
