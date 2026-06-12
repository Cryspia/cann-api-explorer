#!/usr/bin/env bash
# run_one.sh <unit_dir> -- build and run one example unit on card-free (no-NPU) simulation, producing RESULT.md.
#   1) source toolkit set_env (exports ASCEND_TOOLKIT_HOME)
#   2) cmake build (RUN_MODE=sim, SOC=meta.soc_build)
#   3) cannsim record ./sim_exe -s <soc_run> -g -o report
#   4) verify the PASS marker, write the summary report to <unit_dir>/RESULT.md
set -uo pipefail

MINIFORGE_DIR="${MINIFORGE_DIR:-$HOME/miniforge3}"
ENV_NAME="${ENV_NAME:-cannsim}"
SET_ENV="$MINIFORGE_DIR/envs/$ENV_NAME/cann/ascend-toolkit/set_env.sh"

# Tag the per-unit result file by host CPU arch so a run on one host never clobbers
# another host's committed baseline. aarch64 keeps the original unsuffixed name
# (the repo's committed ARM baseline); other arches get an arch suffix.
REPORT_ARCH="${REPORT_ARCH:-$(uname -m)}"
if [ "$REPORT_ARCH" = "aarch64" ]; then RESULT_FILE="RESULT.md"; else RESULT_FILE="RESULT.${REPORT_ARCH}.md"; fi

UNIT="${1:?usage: run_one.sh <unit_dir>}"
UNIT="$(cd "$UNIT" && pwd)"
META="$UNIT/meta.json"
[ -f "$META" ] || { echo "[ERR] $META not found"; exit 2; }

# --- read meta.json (using python, to avoid a jq dependency) ---
read_meta() { python3 -c "import json,sys;print(json.load(open('$META'))['$1'])"; }
NAME="$(read_meta name)"
SOC_BUILD="$(read_meta soc_build)"
SOC_RUN="$(read_meta soc_run)"
PASS_MARKER="$(read_meta pass_marker)"

echo "================ $NAME ================"

# --- environment ---
# shellcheck disable=SC1090
source "$MINIFORGE_DIR/etc/profile.d/conda.sh"
conda activate "$ENV_NAME"
# set_env.sh references several possibly-undefined environment variables; temporarily turn off -u before sourcing
set +u
# shellcheck disable=SC1090
source "$SET_ENV"
set -u
: "${ASCEND_TOOLKIT_HOME:?set_env did not export ASCEND_TOOLKIT_HOME}"

cd "$UNIT"
STATUS="unknown"; INSTR="-"; TIME_NS="-"; BUILD_OK=0; RUN_OK=0

# --- build ---
rm -rf build report
mkdir -p build report
if cmake -S . -B build -DSOC_VERSION="$SOC_BUILD" >build/cmake.log 2>&1 \
   && cmake --build build -j >build/build.log 2>&1; then
    BUILD_OK=1
else
    STATUS="build_failed"
    echo "[FAIL] build failed, see build/build.log"
fi

# --- simulation ---
if [ "$BUILD_OK" = 1 ]; then
    EXE="$UNIT/build/sim_exe"
    if [ -x "$EXE" ]; then
        ( cd report && cannsim record "$EXE" -s "$SOC_RUN" -g -o . ) >report/record.log 2>&1
        if grep -q "$PASS_MARKER" report/record.log; then
            RUN_OK=1; STATUS="passed"
        else
            STATUS="sim_failed"
        fi
        # instruction count / execution time: extracted from the Functions INFO in record.log
        INSTR="$(grep -oE 'instruction executions: [0-9]+' report/record.log | grep -oE '[0-9]+' | tail -1)"
        TIME_NS="$(grep -oE 'Execution time \(ns\): [0-9.]+' report/record.log | grep -oE '[0-9.]+' | tail -1)"
        [ -z "$INSTR" ] && INSTR="-"
        [ -z "$TIME_NS" ] && TIME_NS="-"
    else
        STATUS="no_exe"
    fi
fi

# --- report artifact listing ---
REPORT_FILES="$(cd report 2>/dev/null && find . -type f | sed 's|^\./||' | sort | head -40)"

# --- write RESULT.md ---
{
  echo "# $NAME -- simulation result"
  echo
  echo "- Status: **$STATUS**"
  echo "- Host arch: \`$REPORT_ARCH\`"
  echo "- Build SOC: \`$SOC_BUILD\`  Run SOC: \`$SOC_RUN\`"
  echo "- PASS marker: \`$PASS_MARKER\`"
  echo "- Instruction count: $INSTR"
  echo "- Execution time (ns): $TIME_NS"
  echo
  echo "## record log (tail)"
  echo '```'
  tail -25 report/record.log 2>/dev/null
  echo '```'
  echo
  echo "## Report artifacts"
  echo '```'
  echo "$REPORT_FILES"
  echo '```'
} > "$RESULT_FILE"

echo "[$STATUS] $NAME  (instr=$INSTR, time_ns=$TIME_NS)  -> $UNIT/$RESULT_FILE"
[ "$STATUS" = "passed" ]
