# CANN API Explorer

> 中文版 / Chinese: [README.zh-CN.md](README.zh-CN.md)

For each CANN 9.1.0-beta.1 API, write a **minimal runnable example**, run it under **cannsim card-free (no-NPU) simulation** (CAModel), and record a performance report.

## Setup

**System requirements**: Linux **aarch64 (arm64v8+)** or **x86_64**. `install.sh` auto-detects the host CPU via `uname -m` and downloads the matching CANN `.run` package + miniforge installer (override with `ARCH=aarch64|x86_64`). Auto-installing the system libraries (`libnuma1`/`libgomp1`/`libssl-dev`) assumes a **Debian/Ubuntu-family (apt)** distro; on non-apt distros, install those libraries manually.

Results are tagged by host arch so the two platforms never clobber each other: aarch64 keeps the original unsuffixed names (`RESULT.md`, `reports/INDEX.md`); other arches get a suffix (e.g. `RESULT.x86_64.md`, `reports/INDEX.x86_64.md`). The deterministic CAModel metrics (instruction count, execution-time ns) are host-independent — they should match across arches for the same kernel — while the host CPU only affects the simulator's own wall-clock speed.

The environment is built by this directory's [`install.sh`](install.sh), which installs the CANN Toolkit + the cannsim CLI into an isolated conda env `cannsim` (toolkit only, **no NPU driver** — pure card-free simulation):

- `./install.sh` — full install: download the CANN `.run` → miniforge3 → create the conda env → Toolkit + cannsim wheel → smoke test.
- `./install.sh test` — smoke test only: cannsim CLI / camodel checks + **real-operator simulation of the `vector/add` unit** (compiles an AscendC kernel and runs it on CAModel through the harness).
- `./install.sh test-all` — test + **full re-run of all units** (`run_all.sh`).
- `./install.sh uninstall` — remove the conda env (keeps miniforge and the downloaded `.run`).

Env knobs: `ARCH` (default `uname -m`), `ENV_NAME` (default `cannsim`), `PY_VER`, `MINIFORGE_DIR`, `SKIP_REAL_SIM=1` (skip real-op sim), `RUN_ALL=1` (run all units in `test`). The real-operator simulation reuses `harness/run_one.sh examples/ascendc/vector/add`, which is self-contained (sources conda + set_env, builds, records, verifies).

## Reports & docs

- Aggregate table (instr count / time / report links): [`reports/INDEX.md`](reports/INDEX.md) (aarch64) · [`reports/INDEX.x86_64.md`](reports/INDEX.x86_64.md) (x86_64)
- Performance analysis (instr/time comparison + insights): [`reports/PERF.md`](reports/PERF.md)
- Cross-arch comparison (aarch64 vs x86_64): [`reports/PERF.x86_64.md`](reports/PERF.x86_64.md)
- API doc index: [`docs/INDEX.md`](docs/INDEX.md)

## Arity families covered (9)

- **binary** `(dst,src0,src1,count)`: Add/Sub/Mul/Div/Max/Min
- **scalar** `(dst,src,scalar,count)`: Adds/Muls/Subs/Divs/Maxs/Mins
- **unary** `(dst,src,count)`: Exp/Ln/Abs/Sqrt/Rsqrt/Reciprocal/Relu/Neg
- **cast** `(dst,src,roundMode,count)`: Cast (float→int32)
- **reduce** `(dst,src,tmp,count)`: ReduceSum/ReduceMax/ReduceMin
- **activation / math** (adv_api simple count mode): Sigmoid/Gelu/Silu/Swish + unary Sin/Cos/Tan/Tanh/Sinh/Cosh/Asin/Acos/Atan/Erf/Erfc/Floor/Ceil/Round/Rint/Trunc/Sign/Frac + binary Power/Fmod/Hypot
- **high-level + hand-filled tiling**: RmsNorm / LayerNorm / GroupNorm / DeepNorm / BatchNorm / Pad (a few tiling fields hand-filled inside the kernel for a fixed shape)
- **high-level + device tiling**: SoftMax (`SoftMaxTilingFunc`), LogSoftmax (reuses SoftMaxTilingFunc to fill an isomorphic tiling; measured to use log10), Broadcast (`GetBroadcastTilingInfo`) (tiling computed inside the kernel)
- **multi-core sync**: SyncAll (all-core barrier), IBSet/IBWait (cross-core one-to-one flag, chained dependency) (blockDim=8, DataCopy for GM communication)
- **high-level + host tiling**: TopK (host `TopKTilingFunc` computes → passed in via GM)
- **high-level tiling-free**: Sort (count mode, ascending), Transpose (vtranspose b16, 16×16)
- **cube + host tiling**: Matmul (host `matmul_tiling` computes TCubeTiling → passed in via GM → Cube)
- **manual** (hand-written): Select (+CompareScalar), DataCopy

## Structure

```
manifest.yaml              # source of truth: each API's arity/dtype/inputs/expectation/status
harness/
  templates/*.in           # CMakeLists + kernel/host templates per arity
  gen.py                   # manifest -> examples/<lib>/<cat>/<api>/{kernel,main,CMakeLists,meta,doc}
  run_one.sh <dir>         # set_env -> build -> cannsim record -g -> RESULT.md
  run_all.sh [--gen] [..]  # batch build + simulate
  aggregate.py             # aggregate -> reports/INDEX.md
examples/<lib>/<cat>/<api>/  # one unit per API (doc.md/doc.zh-CN.md/kernel.cpp/main.cpp/CMakeLists.txt/meta.json/RESULT.md/report/)
docs/INDEX.md              # human-readable API index
```

Every example unit is uniform: float dtype, 8 cores, double buffering, build SOC `Ascend950PR_9599`, simulation SOC `Ascend950`.
The function prototypes in doc.md are **extracted from the toolkit headers** by `gen.py` (authoritative, offline) rather than hand-guessed.

## How to add an API

1. Add an entry to `manifest.yaml` (pick the arity, fill inputs/expect; for a new signature, check the toolkit header first).
2. `python3 harness/gen.py <Name>` to generate the unit.
3. `bash harness/run_one.sh examples/.../<name>` to build and simulate.
4. `python3 harness/aggregate.py` to refresh the aggregate table.

A new arity (different signature) needs a template under `harness/templates/` and registration in `gen.py`'s `ARITY_MAP`.

## Scope & completion

- **Track A (simulatable Ascend C kernel API) = core ✅ done (84/84)**: 9 arity families + 4 tiling techniques (device TilingFunc / few-field hand-fill / host-tiling framework / tiling-free) + Cube (Matmul) + multi-core sync (SyncAll/IBSet/IBWait) + data-rearrange/atomic/scalar. The verifiable kernel APIs of this card-free 3510 environment are exhausted; uncovered items (including the arch-limited Conv3D/Conv2D) are itemized in [`docs/INDEX.md`](docs/INDEX.md) under "Track A API coverage wrap-up".
- **Track B (Runtime/ACL host API) = ✅ done**: the host-side scaffolding that launches kernels, systematically documented in [`docs/runtime/host_api.md`](docs/runtime/host_api.md).
- **Track C (GE/HCCL/HIXL/DVPP/ATB/SiP) = documented as not covered**: per-library "why not covered + future conditions" in [`docs/notcovered.md`](docs/notcovered.md) (needs a real driver / multiple cards / dedicated hardware).
- **Pending real hardware**: Conv3D/Conv2D (the toolkit only ships the m220 implementation; needs a 910B-series environment to validate).
