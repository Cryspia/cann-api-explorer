#!/usr/bin/env bash
#
# install.sh — set up the CANN Simulator (card-free / no-NPU simulation) runtime on this machine
#
# Full flow (every step is idempotent and is auto-skipped if already done):
#   1. Download the full CANN package (Ascend-cann_*.run)
#   2. Install miniforge3 (does not write to PATH / does not run conda init)
#   3. Create a dedicated conda env
#   4. Install the CANN Toolkit (toolkit only, no NPU driver) into that env's directory
#   5. Install the cannsim CLI (wheel) + required Python runtime dependencies into that env
#   6. Smoke test + real operator simulation (reusing the api explorer harness: compile an AscendC kernel and run it on CAModel)
#
# Usage:
#   ./install.sh                # install + test (default)
#   ./install.sh install        # same as above
#   ./install.sh test           # run tests only (smoke + vector/add real operator simulation)
#   ./install.sh test-all       # test + full rerun of all api explorer units (run_all, takes a while)
#   ./install.sh uninstall      # remove the conda env (keeps miniforge itself and the downloaded package)
#   ./install.sh --help
#
# Optional environment variable overrides:
#   ARCH=$(uname -m)            # host CPU arch (aarch64 | x86_64); picks the CANN .run + miniforge installer
#   ENV_NAME=cannsim            # conda env name
#   PY_VER=3.10                 # python version of the env
#   MINIFORGE_DIR=$HOME/miniforge3
#   ALLOW_APT=0                 # set to 1 to allow the script to auto sudo apt install missing system libraries
#   SKIP_REAL_SIM=0             # set to 1 to skip the real operator simulation (lightweight smoke only)
#   RUN_ALL=0                   # set to 1 (or use test-all): real operator simulation runs everything instead of just add
#
set -euo pipefail

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Host CPU architecture (aarch64 | x86_64). Both the CANN .run package and the
# miniforge installer use this exact token in their file names, so it drives the
# download URLs below. Override with ARCH=... only for cross-arch debugging.
ARCH="${ARCH:-$(uname -m)}"
case "$ARCH" in
    aarch64|arm64)  ARCH="aarch64" ;;
    x86_64|amd64)   ARCH="x86_64" ;;
    *) echo "[ERROR] unsupported CPU architecture: $ARCH (supported: aarch64, x86_64)" >&2; exit 1 ;;
esac

CANN_URL="https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/CANN/CANN%209.1.T1/Ascend-cann_9.1.0-beta.1_linux-${ARCH}.run"
CANN_PKG="Ascend-cann_9.1.0-beta.1_linux-${ARCH}.run"
# Download/keep the .run inside this repo directory (it is gitignored via *.run).
CANN_RUN="${SCRIPT_DIR}/${CANN_PKG}"
# Compatibility: only reuse a copy already sitting in the parent directory; new
# downloads always go to the repo directory above, never the parent.
[ -f "$CANN_RUN" ] || [ ! -f "${SCRIPT_DIR}/../${CANN_PKG}" ] || CANN_RUN="${SCRIPT_DIR}/../${CANN_PKG}"

MINIFORGE_DIR="${MINIFORGE_DIR:-$HOME/miniforge3}"
MINIFORGE_URL="https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-${ARCH}.sh"

ENV_NAME="${ENV_NAME:-cannsim}"
PY_VER="${PY_VER:-3.10}"
ALLOW_APT="${ALLOW_APT:-0}"

# Default simulation chip used for testing (cannsim currently only supports Ascend950 / Ascend960)
# If the user explicitly sets TEST_SOC it is treated as a forced override; otherwise the script auto-picks a chip whose camodel actually exists
TEST_SOC_OVERRIDE="${TEST_SOC:-}"
TEST_SOC="${TEST_SOC:-Ascend960}"

# Real operator simulation: directly reuse this directory's api explorer harness (run_one does cmake+record+verify itself and sources the environment itself)
HARNESS_DIR="${SCRIPT_DIR}/harness"
SKIP_REAL_SIM="${SKIP_REAL_SIM:-0}"
RUN_ALL_SIM="${RUN_ALL:-0}"   # set to 1: real operator simulation runs everything (all run_all units), otherwise only the vector/add smoke test

# ----------------------------------------------------------------------------
# Output helpers
# ----------------------------------------------------------------------------
c_reset=$'\033[0m'; c_grn=$'\033[92m'; c_yel=$'\033[93m'; c_red=$'\033[91m'; c_cyn=$'\033[96m'
log()  { echo "${c_grn}[INSTALL]${c_reset} $*"; }
step() { echo; echo "${c_cyn}==== $* ====${c_reset}"; }
warn() { echo "${c_yel}[WARN]${c_reset} $*" >&2; }
err()  { echo "${c_red}[ERROR]${c_reset} $*" >&2; }
die()  { err "$@"; exit 1; }

# ----------------------------------------------------------------------------
# Utility functions
# ----------------------------------------------------------------------------
conda_sh() { echo "${MINIFORGE_DIR}/etc/profile.d/conda.sh"; }

# Load conda without polluting the current shell / without relying on PATH
load_conda() {
    [ -f "$(conda_sh)" ] || die "conda.sh not found: ${MINIFORGE_DIR} -- please run install first"
    # shellcheck disable=SC1090
    source "$(conda_sh)"
}

env_prefix() {
    load_conda
    conda env list | awk -v n="$ENV_NAME" '$1==n {print $NF}' | head -1
}

env_exists() { [ -n "$(env_prefix)" ]; }

# ----------------------------------------------------------------------------
# Step 1: download the CANN package
# ----------------------------------------------------------------------------
download_cann() {
    step "Step 1/6: obtain the CANN installer package"
    if [ -f "$CANN_RUN" ]; then
        log "already present, skipping download: $CANN_RUN ($(du -h "$CANN_RUN" | cut -f1))"
        return
    fi
    log "downloading: $CANN_URL"
    if command -v curl >/dev/null 2>&1; then
        curl -fL -C - -o "$CANN_RUN" "$CANN_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -c -O "$CANN_RUN" "$CANN_URL"
    else
        die "neither curl nor wget found, cannot download"
    fi
    chmod +x "$CANN_RUN"
    log "download complete: $CANN_RUN"
}

# ----------------------------------------------------------------------------
# Step 2: install miniforge3 (does not write PATH)
# ----------------------------------------------------------------------------
install_miniforge() {
    step "Step 2/6: install miniforge3"
    if [ -x "${MINIFORGE_DIR}/bin/conda" ]; then
        log "already present, skipping: $MINIFORGE_DIR"
        return
    fi
    local installer="/tmp/Miniforge3-Linux-${ARCH}.sh"
    log "downloading the miniforge installer..."
    if command -v curl >/dev/null 2>&1; then
        curl -fL -o "$installer" "$MINIFORGE_URL"
    else
        wget -O "$installer" "$MINIFORGE_URL"
    fi
    # -b for batch install; does not run conda init, so it will not write to any shell rc / PATH
    bash "$installer" -b -p "$MINIFORGE_DIR"
    rm -f "$installer"
    log "miniforge install complete (not added to PATH): $MINIFORGE_DIR"
}

# ----------------------------------------------------------------------------
# Step 3: create the conda env
# ----------------------------------------------------------------------------
create_env() {
    step "Step 3/6: create conda env '${ENV_NAME}' (python ${PY_VER})"
    load_conda
    if env_exists; then
        log "env already exists, reusing: $(env_prefix)"
        return
    fi
    conda create -y -n "$ENV_NAME" "python=${PY_VER}"
    log "env created: $(env_prefix)"
}

# ----------------------------------------------------------------------------
# Step 4: install the CANN Toolkit into the env directory (toolkit only, no driver)
# ----------------------------------------------------------------------------
toolkit_dir() { echo "$(env_prefix)/cann"; }

# Pick the top-level Toolkit set_env.sh: it must actually export ASCEND_TOOLKIT_HOME,
# and among the matches take the one with the shortest path (i.e. the top-level one, not the ones embedded in components)
find_set_env() {
    find "$(toolkit_dir)" -name set_env.sh 2>/dev/null \
        | while read -r f; do
            if grep -q 'ASCEND_TOOLKIT_HOME=' "$f" 2>/dev/null; then
                echo "${#f} $f"
            fi
          done \
        | sort -n | head -1 | cut -d' ' -f2-
}

install_toolkit() {
    step "Step 4/6: install the CANN Toolkit into the env directory"
    [ -f "$CANN_RUN" ] || die "missing $CANN_RUN, please run download first"
    load_conda; conda activate "$ENV_NAME"

    local tdir; tdir="$(toolkit_dir)"
    if [ -n "$(find_set_env)" ]; then
        log "Toolkit already installed, skipping: $(find_set_env)"
        return
    fi

    mkdir -p "$tdir"
    log "install target: $tdir"
    log "mode: --full --whitelist=toolkit (does not install the NPU driver)"
    chmod +x "$CANN_RUN" 2>/dev/null || true
    # The Ascend installer writes its security/operation logs to $HOME/var/log/ascend_seclog
    # by default, scattering an Ascend dir under the user's HOME. The log root follows $HOME,
    # so point HOME at the toolkit dir for just this command -> logs land under the env
    # (removed on uninstall) and nothing is created in the real HOME.
    local install_home="${tdir}/install_home"
    mkdir -p "$install_home"
    # the env is activated -> the pip packages bundled in the installer land in this env's site-packages (not the system python)
    HOME="$install_home" bash "$CANN_RUN" --full --whitelist=toolkit --install-path="$tdir" --quiet

    local se; se="$(find_set_env)"
    [ -n "$se" ] || die "set_env.sh not found after Toolkit install, the install may have failed"
    log "Toolkit install complete, set_env.sh: $se"
}

# Safely source the top-level set_env.sh: it references several possibly-undefined variables,
# which would abort sourcing under set -u, so pre-set them to empty and temporarily turn off -u
source_setenv() {
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
    export PYTHONPATH="${PYTHONPATH:-}"
    export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
    export PATH="${PATH:-}"
    set +u
    # shellcheck disable=SC1090
    source "$1"
    set -u
}

# cannsim-supported chip -> simulation model name inside the Toolkit (see cannsim/core/soc_info.py)
soc_to_model() {
    case "$1" in
        Ascend950) echo "Ascend950PR_9599" ;;
        Ascend960) echo "dav_9201" ;;
        *)         echo "" ;;
    esac
}

# Return a chip's corresponding camodel directory (only if it exists); requires ASCEND_TOOLKIT_HOME to be set
camodel_for_soc() {
    local m d
    m="$(soc_to_model "$1")"; [ -n "$m" ] || return 1
    d="${ASCEND_TOOLKIT_HOME:-}/tools/simulator/${m}/camodel"
    [ -d "$d" ] && echo "$d"
}

# Auto-pick a supported chip whose camodel actually exists (a user TEST_SOC override takes priority)
auto_pick_soc() {
    if [ -n "${TEST_SOC_OVERRIDE:-}" ]; then echo "$TEST_SOC_OVERRIDE"; return; fi
    local s
    for s in Ascend960 Ascend950; do
        [ -n "$(camodel_for_soc "$s")" ] && { echo "$s"; return; }
    done
    echo "$TEST_SOC"
}

# ----------------------------------------------------------------------------
# Step 5: install the cannsim CLI (wheel) + Python runtime dependencies
# ----------------------------------------------------------------------------
# Find the cannsim wheel: bundled inside the toolkit installed from the full CANN package
# (.../cann/<ver>/<arch>-linux/simulator/bin/cannsim-*.whl); take it only from the toolkit.
locate_cannsim_wheel() {
    find "$(toolkit_dir)" -name 'cannsim-*.whl' 2>/dev/null | head -1
}

install_python_pkgs() {
    step "Step 5/6: install the cannsim CLI and Python runtime dependencies (prefer installing into the env)"
    load_conda; conda activate "$ENV_NAME"

    # 5a) cannsim CLI
    if python -c "import cannsim" 2>/dev/null; then
        log "cannsim already installed, skipping wheel"
    else
        local whl; whl="$(locate_cannsim_wheel)"
        [ -n "$whl" ] || die "cannsim wheel not found in the toolkit, please complete step 4 first (install the toolkit from the full CANN package)"
        log "installing cannsim wheel: $whl"
        pip install --no-input "$whl"
    fi

    # 5b) optional dependency for the report feature (HTML reports need plotly); its absence does not affect record
    if ! python -c "import plotly" 2>/dev/null; then
        log "installing optional dependency plotly (for report HTML charts)"
        pip install --no-input plotly || warn "plotly install failed, the report's HTML charts will be unavailable (does not affect record)"
    fi

    # 5c) common C/C++ runtime libraries: prefer installing into the env with conda (satisfies the camodel .so dependencies)
    log "ensuring the C++/OpenMP runtime libraries are in the env (conda-forge)"
    conda install -y -n "$ENV_NAME" -c conda-forge libstdcxx-ng libgcc-ng libgomp >/dev/null 2>&1 \
        || warn "conda runtime library install skipped (may already be satisfied)"
}

# ----------------------------------------------------------------------------
# Runtime library self-check: check which system libraries the camodel .so is missing
# ----------------------------------------------------------------------------
# Arg $1 = the camodel directory of the chip to check (passed in by smoke_test, env already sourced)
check_runtime_libs() {
    local camodel="${1:-}"
    [ -n "$camodel" ] && [ -d "$camodel" ] || { warn "no camodel directory, skipping runtime library self-check"; return 0; }
    # Key point: both the camodel directory and its sibling lib/ directory must be on LD_LIBRARY_PATH,
    # otherwise same-package libraries that libcamodel.so depends on (e.g. libnpu_drv_pvmodel.so) would be misreported as "not found"
    export LD_LIBRARY_PATH="${camodel}:${camodel}/../lib:${CONDA_PREFIX:-}/lib:${LD_LIBRARY_PATH:-}"
    local missing
    missing="$(find "$camodel" -maxdepth 1 -name '*.so' -exec ldd {} \; 2>/dev/null \
                | awk '/not found/{print $1}' | sort -u)"
    if [ -z "$missing" ]; then
        log "runtime library self-check passed: all dynamic libraries the camodel depends on can be resolved"
        return 0
    fi
    warn "the camodel is missing the following system libraries:"
    echo "$missing" | sed 's/^/    - /'
    # try to map them to apt packages
    local aptpkgs="" l
    for l in $missing; do
        case "$l" in
            libnuma*)      aptpkgs="$aptpkgs libnuma1" ;;
            libgomp*)      aptpkgs="$aptpkgs libgomp1" ;;
            libssl*|libcrypto*) aptpkgs="$aptpkgs libssl-dev" ;;
            *)             ;;
        esac
    done
    aptpkgs="$(echo "$aptpkgs" | xargs -n1 2>/dev/null | sort -u | xargs)"
    if [ -n "$aptpkgs" ]; then
        warn "suggest installing with the system package manager: sudo apt-get install -y ${aptpkgs}"
        if [ "$ALLOW_APT" = "1" ]; then
            warn ">>> ALLOW_APT=1, the script will run sudo apt to install the system libraries (requires the sudo password) <<<"
            sudo apt-get update && sudo apt-get install -y $aptpkgs || warn "apt install failed, please handle manually"
        else
            warn ">>> by default it does not run sudo apt automatically. Once you confirm, rerun: ALLOW_APT=1 ./install.sh test"
        fi
    else
        warn "based on the missing library names above, install the corresponding system packages with sudo apt (the script does not sudo automatically by default)"
    fi
}

# ----------------------------------------------------------------------------
# Real operator simulation: compile the AscendC AddCustom kernel -> cannsim record runs it on CAModel
# Before this is called the env is activated, set_env.sh is sourced, and ASCEND_TOOLKIT_HOME is set
# ----------------------------------------------------------------------------
real_op_simulation() {
    log "additional test: real operator simulation (reuse api explorer harness -> CAModel)"
    if [ "$SKIP_REAL_SIM" = "1" ]; then warn "  SKIP_REAL_SIM=1, skipping real operator simulation"; return 0; fi
    command -v cmake >/dev/null 2>&1 || { warn "  cmake missing, skipping real operator simulation"; return 0; }
    [ -f "$HARNESS_DIR/run_one.sh" ] || { warn "  $HARNESS_DIR/run_one.sh not found, skipping"; return 0; }

    if [ "$RUN_ALL_SIM" = "1" ]; then
        # full run: rerun all api explorer units (run_one does cmake+record+verify itself, run_all aggregates reports/INDEX.md at the end)
        log "  RUN_ALL=1: full rerun of all units (takes a while)..."
        if bash "$HARNESS_DIR/run_all.sh"; then
            log "  OK full simulation complete (summary in $SCRIPT_DIR/reports/INDEX.md)"
        else
            warn "  full simulation has failures (see $SCRIPT_DIR/reports/INDEX.md)"
        fi
    else
        # smoke: only run the vector/add unit (z = x + y). run_one sources conda+set_env itself, no external environment needed
        log "  running the vector/add unit (run_one does cmake+record+verify itself, takes tens of seconds)..."
        if bash "$HARNESS_DIR/run_one.sh" "$SCRIPT_DIR/examples/ascendc/vector/add"; then
            log "  OK real operator simulation passed: z = x + y result verified correct"
            log "    (more units via ./install.sh test-all or $HARNESS_DIR/run_all.sh)"
        else
            warn "  real operator simulation did not pass (see $SCRIPT_DIR/examples/ascendc/vector/add/RESULT.md)"
        fi
    fi
}

# ----------------------------------------------------------------------------
# Step 6: smoke test
# ----------------------------------------------------------------------------
smoke_test() {
    step "Step 6/6: smoke test"
    load_conda; conda activate "$ENV_NAME"

    local se; se="$(find_set_env)"
    [ -n "$se" ] || die "Toolkit set_env.sh not found, run install first"
    source_setenv "$se"
    : "${ASCEND_TOOLKIT_HOME:=}"
    log "set_env.sh = $se"
    log "ASCEND_TOOLKIT_HOME = ${ASCEND_TOOLKIT_HOME:-<not set>}"
    [ -n "$ASCEND_TOOLKIT_HOME" ] || die "set_env.sh did not export ASCEND_TOOLKIT_HOME, cannot continue"

    # pick a supported chip whose camodel actually exists
    TEST_SOC="$(auto_pick_soc)"
    log "test chip: ${TEST_SOC} (model $(soc_to_model "$TEST_SOC"))"

    # 6a) cannsim CLI
    log "testing cannsim --help"
    cannsim --help >/dev/null && log "  OK cannsim CLI runs"

    # 6b) cannprof binary
    local cannprof
    cannprof="$(python -c "import cannsim,os;print(os.path.join(os.path.dirname(cannsim.__file__),'bin','cannprof'))")"
    if [ -x "$cannprof" ]; then
        "$cannprof" --help >/dev/null 2>&1 && log "  OK cannprof binary runs ($(basename "$cannprof"))"
    fi

    # 6c) camodel library existence (test chip)
    local camodel; camodel="$(camodel_for_soc "$TEST_SOC" || true)"
    if [ -n "$camodel" ]; then
        log "  OK ${TEST_SOC} simulation library ready: $camodel"
    else
        warn "  camodel directory for ${TEST_SOC}($(soc_to_model "$TEST_SOC")) not found"
    fi

    # 6d) runtime library self-check (pass in the test chip's camodel)
    check_runtime_libs "$camodel"

    # 6e) end-to-end record (run the full record pipeline with a minimal executable)
    log "running record once with a placeholder program (verifies the record pipeline, not a real kernel)"
    local work; work="$(mktemp -d)"
    cat > "$work/dummy_app" <<'EOF'
#!/usr/bin/env bash
echo "[dummy kernel] hello from cann simulator smoke test"
EOF
    chmod +x "$work/dummy_app"
    if cannsim record "$work/dummy_app" -s "$TEST_SOC" -o "$work/out" > "$work/reclog" 2>&1; then
        log "  OK record pipeline ran successfully (output in $work/out)"
    else
        warn "  record returned non-zero (the placeholder program is not a real AscendC kernel, which is expected; see $work/reclog)"
        grep -iE 'error|fail' "$work/reclog" | head -5 | sed 's/^/      /' || true
    fi

    # 6f) real operator simulation (compile an AscendC kernel and run it on CAModel)
    echo
    real_op_simulation

    echo
    log "${c_grn}smoke test finished.${c_reset}"
    echo
    echo "To use this environment manually next time (no need to add miniforge to PATH):"
    echo "  source ${MINIFORGE_DIR}/etc/profile.d/conda.sh"
    echo "  conda activate ${ENV_NAME}"
    echo "  source $(find_set_env)"
    echo "  cannsim record <your AscendC executable> -s ${TEST_SOC} -o ./out -g"
}

# ----------------------------------------------------------------------------
# Uninstall: only remove the env (keeps miniforge itself and the downloaded package)
# ----------------------------------------------------------------------------
do_uninstall() {
    step "uninstall: remove conda env '${ENV_NAME}' (keeps miniforge and the downloaded package)"
    load_conda
    if ! env_exists; then
        log "env '${ENV_NAME}' does not exist, nothing to remove"
        return
    fi
    local pfx; pfx="$(env_prefix)"
    conda deactivate 2>/dev/null || true
    conda env remove -y -n "$ENV_NAME"
    # the Toolkit is installed under the env directory and is removed along with the env; clean once more in case of leftovers
    [ -d "$pfx" ] && rm -rf "$pfx"
    log "env removed: $pfx"
    log "kept: miniforge ($MINIFORGE_DIR), CANN installer package ($CANN_RUN)"
}

# ----------------------------------------------------------------------------
# Main flow
# ----------------------------------------------------------------------------
do_install() {
    download_cann
    install_miniforge
    create_env
    install_toolkit
    install_python_pkgs
    smoke_test
}

usage() {
    sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'
}

main() {
    case "${1:-install}" in
        install|"")   do_install ;;
        test)         smoke_test ;;
        test-all)     RUN_ALL_SIM=1; smoke_test ;;
        uninstall)    do_uninstall ;;
        -h|--help|help) usage ;;
        *) die "unknown command: $1 (available: install | test | test-all | uninstall)" ;;
    esac
}

main "$@"
