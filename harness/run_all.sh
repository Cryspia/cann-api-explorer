#!/usr/bin/env bash
# run_all.sh [--gen] [name...] -- batch build + simulate all (or selected) units, then aggregate reports/INDEX.md.
#   --gen        first run python3 harness/gen.py (regenerate units from the manifest), then run
#   name...      run only the selected APIs (directory name, case-insensitive); defaults to everything under examples
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

GEN=0
ARGS=()
for a in "$@"; do
    if [ "$a" = "--gen" ]; then GEN=1; else ARGS+=("$a"); fi
done

if [ "$GEN" = 1 ]; then
    echo ">>> regenerating units"
    source "${MINIFORGE_DIR:-$HOME/miniforge3}/etc/profile.d/conda.sh"
    conda activate "${ENV_NAME:-cannsim}"
    python3 harness/gen.py "${ARGS[@]}"
fi

# collect unit directories
mapfile -t UNITS < <(find examples -name meta.json -printf '%h\n' | sort)
if [ "${#ARGS[@]}" -gt 0 ]; then
    SEL=()
    for u in "${UNITS[@]}"; do
        base="$(basename "$u")"
        for a in "${ARGS[@]}"; do
            [ "${base,,}" = "${a,,}" ] && SEL+=("$u")
        done
    done
    UNITS=("${SEL[@]}")
fi

echo ">>> units to run: ${#UNITS[@]}"
pass=0; fail=0
for u in "${UNITS[@]}"; do
    if bash harness/run_one.sh "$u"; then
        pass=$((pass+1))
    else
        fail=$((fail+1))
    fi
done

echo ">>> aggregating report"
source "${MINIFORGE_DIR:-$HOME/miniforge3}/etc/profile.d/conda.sh"
conda activate "${ENV_NAME:-cannsim}"
python3 harness/aggregate.py

echo ">>> done: $pass passed / $fail failed (${#UNITS[@]} total)"
