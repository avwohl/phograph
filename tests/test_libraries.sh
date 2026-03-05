#!/bin/bash
# Test suite for Phograph library system
# Verifies: manifests parse, all expected libraries exist, primitive counts match,
# example JSON is valid, and primitives are registered in the engine.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LIB_DIR="$PROJECT_DIR/libraries"
PASS=0
FAIL=0

pass() { PASS=$((PASS + 1)); echo "  PASS: $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

echo "=== Phograph Library Test Suite ==="
echo ""

# -------------------------------------------------------
# Test 1: All expected libraries exist
# -------------------------------------------------------
echo "--- Test: Library directories exist ---"
EXPECTED_LIBS="bitmap crypto fileio image locale math midi net socket sound"
for lib in $EXPECTED_LIBS; do
    if [ -d "$LIB_DIR/$lib" ]; then
        pass "$lib/ directory exists"
    else
        fail "$lib/ directory missing"
    fi
done

# -------------------------------------------------------
# Test 2: All library.json files are valid JSON
# -------------------------------------------------------
echo ""
echo "--- Test: library.json files parse as valid JSON ---"
for lib in $EXPECTED_LIBS; do
    manifest="$LIB_DIR/$lib/library.json"
    if [ -f "$manifest" ]; then
        if python3 -c "import json; json.load(open('$manifest'))" 2>/dev/null; then
            pass "$lib/library.json is valid JSON"
        else
            fail "$lib/library.json is invalid JSON"
        fi
    else
        fail "$lib/library.json not found"
    fi
done

# -------------------------------------------------------
# Test 3: Required manifest fields present
# -------------------------------------------------------
echo ""
echo "--- Test: Required manifest fields ---"
for lib in $EXPECTED_LIBS; do
    manifest="$LIB_DIR/$lib/library.json"
    if [ -f "$manifest" ]; then
        result=$(python3 -c "
import json, sys
m = json.load(open('$manifest'))
missing = [f for f in ['name','version','description','primitives'] if f not in m]
if missing:
    print('MISSING:' + ','.join(missing))
else:
    print('OK')
" 2>/dev/null)
        if [ "$result" = "OK" ]; then
            pass "$lib has all required fields"
        else
            fail "$lib $result"
        fi
    fi
done

# -------------------------------------------------------
# Test 4: Primitive names are unique across libraries
# -------------------------------------------------------
echo ""
echo "--- Test: Primitive names globally unique ---"
DUPES=$(python3 -c "
import json, os, sys
seen = {}
dupes = []
for lib in '$EXPECTED_LIBS'.split():
    path = os.path.join('$LIB_DIR', lib, 'library.json')
    m = json.load(open(path))
    for p in m['primitives']:
        name = p['name']
        if name in seen:
            dupes.append(f'{name} (in {seen[name]} and {lib})')
        seen[name] = lib
if dupes:
    print('\\n'.join(dupes))
else:
    print('OK')
" 2>/dev/null)
if [ "$DUPES" = "OK" ]; then
    pass "All primitive names globally unique"
else
    fail "Duplicate primitives: $DUPES"
fi

# -------------------------------------------------------
# Test 5: Primitive counts per library
# -------------------------------------------------------
echo ""
echo "--- Test: Primitive counts ---"
COUNT_RESULT=$(python3 -c "
import json, os
expected = {'bitmap':21,'crypto':17,'fileio':22,'image':15,'locale':12,'math':5,'midi':13,'net':3,'socket':13,'sound':14}
for lib in sorted(expected):
    path = os.path.join('$LIB_DIR', lib, 'library.json')
    m = json.load(open(path))
    actual = len(m['primitives'])
    exp = expected[lib]
    if actual == exp:
        print(f'PASS:{lib} has {actual} primitives (expected {exp})')
    else:
        print(f'FAIL:{lib} has {actual} primitives (expected {exp})')
" 2>/dev/null)
while IFS= read -r line; do
    status="${line%%:*}"
    msg="${line#*:}"
    if [ "$status" = "PASS" ]; then
        pass "$msg"
    else
        fail "$msg"
    fi
done <<< "$COUNT_RESULT"

# -------------------------------------------------------
# Test 6: Primitive num_inputs/num_outputs are valid
# -------------------------------------------------------
echo ""
echo "--- Test: Primitive input/output counts valid ---"
BAD=$(python3 -c "
import json, os
bad = []
for lib in '$EXPECTED_LIBS'.split():
    m = json.load(open(os.path.join('$LIB_DIR', lib, 'library.json')))
    for p in m['primitives']:
        ni = p.get('num_inputs', -1)
        no = p.get('num_outputs', -1)
        if not isinstance(ni, int) or ni < 0 or not isinstance(no, int) or no < 0:
            bad.append(f'{lib}/{p[\"name\"]}')
if bad:
    print(','.join(bad))
else:
    print('OK')
" 2>/dev/null)
if [ "$BAD" = "OK" ]; then
    pass "All primitives have valid input/output counts"
else
    fail "Invalid counts: $BAD"
fi

# -------------------------------------------------------
# Test 7: C++ prim files exist for all libraries
# -------------------------------------------------------
echo ""
echo "--- Test: C++ primitive files exist ---"
CPP_DIR="$PROJECT_DIR/phograph_core/src"
PRIM_FILES="math fileio socket sound midi locale crypto image bitmap net"
for pf in $PRIM_FILES; do
    if [ -f "$CPP_DIR/pho_prim_$pf.cc" ]; then
        pass "pho_prim_$pf.cc exists"
    else
        fail "pho_prim_$pf.cc missing"
    fi
done

# -------------------------------------------------------
# Test 8: Plugin source files exist
# -------------------------------------------------------
echo ""
echo "--- Test: Plugin source files ---"
PLUGIN_DIR="$CPP_DIR/plugins"
for f in SoundPlugin.h SoundPlugin.cpp MIDIPlugin.h MIDIPlugin.cpp; do
    if [ -f "$PLUGIN_DIR/$f" ]; then
        pass "$f exists"
    else
        fail "$f missing"
    fi
done

# -------------------------------------------------------
# Test 9: Registration in pho_prim.cc
# -------------------------------------------------------
echo ""
echo "--- Test: All prims registered in pho_prim.cc ---"
for pf in $PRIM_FILES; do
    if grep -q "register_${pf}_prims()" "$CPP_DIR/pho_prim.cc" 2>/dev/null; then
        pass "register_${pf}_prims() called"
    else
        fail "register_${pf}_prims() not found in pho_prim.cc"
    fi
done

# -------------------------------------------------------
# Test 10: Declarations in pho_prim.h
# -------------------------------------------------------
echo ""
echo "--- Test: All prims declared in pho_prim.h ---"
for pf in $PRIM_FILES; do
    if grep -q "register_${pf}_prims()" "$CPP_DIR/pho_prim.h" 2>/dev/null; then
        pass "register_${pf}_prims() declared"
    else
        fail "register_${pf}_prims() not found in pho_prim.h"
    fi
done

# -------------------------------------------------------
# Test 11: Example catalog references library examples
# -------------------------------------------------------
echo ""
echo "--- Test: ExampleCatalog has library examples ---"
CATALOG="$PROJECT_DIR/Phograph/Model/ExampleCatalog.swift"
if grep -q "libraryExamples" "$CATALOG" 2>/dev/null; then
    pass "libraryExamples referenced in ExampleCatalog"
else
    fail "libraryExamples not found in ExampleCatalog"
fi

# Count library examples
LIB_EXAMPLE_COUNT=$(grep -c '"Libraries"' "$CATALOG" 2>/dev/null || echo 0)
if [ "$LIB_EXAMPLE_COUNT" -ge 1 ]; then
    pass "Libraries category found in ExampleCatalog"
else
    fail "Libraries category not found"
fi

# -------------------------------------------------------
# Summary
# -------------------------------------------------------
echo ""
echo "=== Results ==="
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
TOTAL=$((PASS + FAIL))
echo "  Total:  $TOTAL"
echo ""
if [ "$FAIL" -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
