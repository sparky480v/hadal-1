#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# HADAL-1's tests: the engine for an hour of every control, and the whole app through all nine seas in fm1-base's simulator.
set -eu
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
PY="${PYTHON:-python3}"
OUT=app/build/tests; mkdir -p "$OUT"
"$PY" base/tools/build.py app --host >/dev/null
$CC -O2 -ffp-contract=off -std=c99 -DPLAT_HOST -Iapp -Iapp/dsp -Ibase/include -o "$OUT/soak" tests/host/soak.c app/dsp/hadal.c -lm
"$OUT/soak" 0.25 7 | tee "$OUT/soak.json"
grep -q '"bad":0,' "$OUT/soak.json"
app/build/host/hadal_host tests/scenarios/seas.txt "$OUT"
echo "ok"
