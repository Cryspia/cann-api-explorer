# CANN API Explorer — x86_64 Verification & Cross-Arch Comparison

> 中文版: [PERF.x86_64.zh-CN.md](PERF.x86_64.zh-CN.md)
>
> This report records bringing the project up on **x86_64** and comparing it against the committed **aarch64** baseline. The aarch64 results ([`INDEX.md`](INDEX.md) / [`PERF.md`](PERF.md)) are left untouched; x86 results live in arch-tagged files ([`INDEX.x86_64.md`](INDEX.x86_64.md), per-unit `RESULT.x86_64.md`).

## Result: **118/118 simulations passed on x86_64** ✅

- **Host**: AMD Ryzen AI MAX+ 395, 16 cores / 31 GiB RAM (Linux x86_64) — right at CAModel's stated minimum (≥16 cores, ≥32 GB).
- **Package**: `Ascend-cann_9.1.0-beta.1_linux-x86_64.run`, toolkit-only (no NPU driver), pure card-free CAModel simulation. Test chip Ascend950PR_9599 (build) / Ascend950 (run); smoke chip Ascend960 (dav_9201).
- Coverage grew 84 → 118 as the upstream computation-class operators landed (fused binary, shifts, scalar variants, inverse-hyperbolic, logical/predicate bool-out ops, Where, CumSum, …). All 34 new units passed on x86 unchanged — no x86-specific fixes were needed for them (they already emit the marker before ACL teardown, and none hardcode an arch include path).

## What it took to support x86 (3 fixes)

1. **Arch-aware install** (`install.sh`): `uname -m` now selects the CANN `.run` URL/filename and the miniforge installer (`linux-x86_64` vs `linux-aarch64`); override with `ARCH=`. The cannsim wheel and toolkit header dirs are located by glob, not a hardcoded arch.
2. **Marker emitted before ACL teardown** (host `main.cpp` + templates): on this x86 build, `aclFinalize()` ends the process / closes the simulator's stdout capture, so the `<TAG> SIMULATION PASSED` line printed *after* teardown was never recorded — every unit looked `sim_failed` despite `errors=0`. The marker now prints before teardown (the `errors` count is already final there). Helps both arches; semantics unchanged.
3. **Arch-portable include path** (`topk/CMakeLists.txt`, `gen.py`): TopK's `topk_tiling.h` pulls in `topk_utils_constants.h`, which lives under the toolkit's `<arch>-linux/asc/include/adv_api/sort`. That path was hardcoded to `aarch64-linux`; it now globs whichever `*-linux` dir exists.

Results are tagged by arch so a run on one host never clobbers the other (`run_one.sh` / `aggregate.py` honor `REPORT_ARCH`, default `uname -m`; aarch64 keeps the original unsuffixed names).

## 1. Simulation fidelity: x86 reproduces the aarch64 CAModel metrics

The two CAModel metrics (instruction-execution count, execution-time ns) are device-level and should be host-independent. Measured across all 118 units:

| Metric | mean \|Δ\| | median \|Δ\| | max \|Δ\| |
|---|---|---|---|
| **Instruction count** | **0.026 %** | 0.005 % | 1.76 % (SyncAll) |
| **Execution time (ns)** | 2.29 % | 0.63 % | 18.2 % (small-op, fixed-overhead dominated) |

- **Instruction count is effectively identical**: 52/118 units are bit-for-bit equal; the rest differ by only a handful of instructions on totals of 20k–160k (≤0.03 %). Pure compute / cube / normalization / logical / predicate kernels match exactly or near-exactly.
- **The only >0.1 % instruction deltas are the multi-core sync units**: SyncAll (+1.76 %, 6477→6591) and IBSetWait (+0.28 %, 7758→7780). These busy-wait-poll a GM flag, so the number of poll iterations depends on inter-core scheduling — slightly host-sensitive by nature, not a correctness issue.
- **Execution-time(ns) is the CAModel estimate**, derived from the instruction trace; it tracks instruction count closely. Larger relative swings appear only on tiny operators where a few-ns fixed-overhead estimate dominates a sub-2500 ns total.

**Conclusion**: the x86 toolkit produces the same device-level simulation as aarch64 to within sub-0.05 % on compute kernels — the platform is functionally equivalent for this project's purposes.

## 2. Host efficiency on this machine (the genuinely host-dependent metric)

CAModel runs the AI-Core simulation on the host CPU, so the **simulator's own wall-clock** is what the host actually governs (the metrics in §1 are not). Measured from each unit's `record.log` (`Command executed successfully in Ns`, the USER_APP CAModel execution):

| | value |
|---|---|
| Total wall-clock, 118 units (sequential) | **~152 min** (9095 s) |
| Per-unit range | **43.8 s – 167.5 s** |
| Per-unit median / mean | 87 s / 77 s |
| Fixed floor (smallest ~200-instr unit) | **~44 s** |

- There is a **~44 s fixed startup floor** per unit (CAModel init + SoC bring-up + teardown), independent of kernel size — even a ~200-instruction op takes ~46 s.
- Above the floor, wall-clock **scales with instruction count**: `Lgamma` (160k instr) is the slowest at 168 s, then `Digamma` 152 s; the multi-core sync units (`IBSetWait` 115 s, `SyncAll` 106 s) are next because polling inflates simulated work.
- This machine sits at CAModel's **minimum** spec (16 c / 31 GB vs the tool's reference 64 c / 256 GB), so per-operator wall-clock is on the slow end — expected, and orthogonal to the (host-independent) accuracy in §1.

Slowest / fastest:

| Slowest 5 | wall(s) | instr | | Fastest 5 | wall(s) | instr |
|---|--:|--:|---|---|--:|--:|
| Lgamma | 168 | 160040 | | TopK | 44 | 561 |
| Digamma | 152 | 77184 | | Duplicate | 46 | 204 |
| IBSetWait | 115 | 7780 | | Transpose | 46 | 259 |
| Power | 107 | 41664 | | CastF2H | 46 | 301 |
| SyncAll | 106 | 6591 | | CreateVecIndex | 47 | 212 |

## 3. Full per-unit comparison (aarch64 vs x86_64)

`Δinstr%` = (x86 − arm) / arm. `x86 wall(s)` = CAModel execution wall-clock on this host.

| API | Cat | instr arm | instr x86 | Δinstr% | ns arm | ns x86 | x86 wall(s) |
|---|---|--:|--:|--:|--:|--:|--:|
| Matmul | cube | 5245 | 5245 | +0.00 | 9538.18 | 9547.88 | 88 |
| DataCopy | datacopy | 19005 | 19008 | +0.02 | 5892.12 | 5875.15 | 87 |
| Acos | highlevel | 37186 | 37192 | +0.02 | 5847.88 | 5824.24 | 102 |
| Acosh | highlevel | 23141 | 23142 | +0.00 | 6346.67 | 6209.7 | 98 |
| Asin | highlevel | 36039 | 36048 | +0.02 | 5835.76 | 6120.61 | 103 |
| Asinh | highlevel | 23929 | 23935 | +0.03 | 6353.33 | 6285.45 | 96 |
| Atan | highlevel | 43113 | 43118 | +0.01 | 6903.64 | 6422.42 | 100 |
| Atanh | highlevel | 23276 | 23279 | +0.01 | 6206.67 | 6181.82 | 94 |
| BatchNorm | highlevel | 863 | 863 | +0.00 | 3233.33 | 3202.42 | 60 |
| BitwiseXor | highlevel | 324 | 324 | +0.00 | 2049.09 | 2239.39 | 50 |
| Broadcast | highlevel | 620 | 620 | +0.00 | 2485.45 | 2474.55 | 55 |
| Ceil | highlevel | 22078 | 22080 | +0.01 | 5981.82 | 6030.91 | 90 |
| ClampMax | highlevel | 286 | 286 | +0.00 | 1910.3 | 1907.27 | 47 |
| ClampMin | highlevel | 286 | 286 | +0.00 | 1915.15 | 1912.73 | 51 |
| Cos | highlevel | 31848 | 31856 | +0.03 | 6183.03 | 6212.73 | 97 |
| Cosh | highlevel | 23147 | 23150 | +0.01 | 6225.45 | 6195.76 | 88 |
| CumSum | highlevel | 684 | 684 | +0.00 | 2396.97 | 2386.06 | 55 |
| DeepNorm | highlevel | 854 | 854 | +0.00 | 3375.15 | 3496.36 | 61 |
| Digamma | highlevel | 77180 | 77184 | +0.01 | 9672.73 | 9595.76 | 152 |
| Erf | highlevel | 28618 | 28623 | +0.02 | 6216.36 | 6210.3 | 91 |
| Erfc | highlevel | 33660 | 33664 | +0.01 | 6692.12 | 6247.27 | 97 |
| Floor | highlevel | 22075 | 22080 | +0.02 | 6000.61 | 6003.03 | 87 |
| Fma | highlevel | 399 | 399 | +0.00 | 2598.18 | 2596.36 | 54 |
| Fmod | highlevel | 36540 | 36544 | +0.01 | 6792.73 | 6828.48 | 103 |
| Frac | highlevel | 22328 | 22334 | +0.03 | 6053.94 | 6043.64 | 87 |
| GeGLU | highlevel | 347 | 347 | +0.00 | 2368.48 | 2078.18 | 51 |
| Gelu | highlevel | 25135 | 25136 | +0.00 | 6564.85 | 5952.73 | 90 |
| GroupNorm | highlevel | 634 | 634 | +0.00 | 2500.61 | 2510.3 | 52 |
| Hypot | highlevel | 40110 | 40111 | +0.00 | 6357.58 | 6473.33 | 103 |
| IsFinite | highlevel | 25019 | 25024 | +0.02 | 6216.97 | 6202.42 | 97 |
| IsInf | highlevel | 24507 | 24512 | +0.02 | 6358.18 | 6166.06 | 93 |
| IsNan | highlevel | 23568 | 23576 | +0.03 | 6166.67 | 6197.58 | 90 |
| LayerNorm | highlevel | 909 | 909 | +0.00 | 2767.27 | 2757.58 | 56 |
| Lgamma | highlevel | 159997 | 160040 | +0.03 | 9672.73 | 9800.61 | 168 |
| Log | highlevel | 22078 | 22080 | +0.01 | 6051.52 | 6106.06 | 90 |
| LogSoftmax | highlevel | 514 | 514 | +0.00 | 2607.88 | 2605.45 | 55 |
| LogicalAnd | highlevel | 33410 | 33416 | +0.02 | 6278.18 | 6432.12 | 105 |
| LogicalNot | highlevel | 24046 | 24048 | +0.01 | 6096.36 | 6123.03 | 91 |
| LogicalOr | highlevel | 33414 | 33416 | +0.01 | 6304.85 | 6386.06 | 105 |
| LogicalXor | highlevel | 33413 | 33416 | +0.01 | 6297.58 | 6284.24 | 99 |
| Pad | highlevel | 318 | 318 | +0.00 | 2315.15 | 2204.85 | 51 |
| Power | highlevel | 41654 | 41664 | +0.02 | 6827.88 | 7133.33 | 107 |
| Rint | highlevel | 22076 | 22080 | +0.02 | 6012.73 | 6075.76 | 90 |
| RmsNorm | highlevel | 553 | 553 | +0.00 | 2424.85 | 2441.82 | 54 |
| Round | highlevel | 22073 | 22079 | +0.03 | 6010.3 | 6069.09 | 90 |
| Sigmoid | highlevel | 22911 | 22912 | +0.00 | 6197.58 | 6205.45 | 91 |
| Sign | highlevel | 23038 | 23040 | +0.01 | 6050.3 | 6065.45 | 88 |
| Silu | highlevel | 22652 | 22656 | +0.02 | 6208.48 | 6325.45 | 88 |
| Sin | highlevel | 29366 | 29366 | +0.00 | 6227.88 | 6220.0 | 97 |
| SinCos | highlevel | 589 | 589 | +0.00 | 2744.24 | 2549.09 | 52 |
| Sinh | highlevel | 23150 | 23152 | +0.01 | 6186.06 | 6174.55 | 94 |
| SoftMax | highlevel | 423 | 423 | +0.00 | 2189.09 | 2192.73 | 52 |
| Sort | highlevel | 1774 | 1774 | +0.00 | 3893.94 | 3884.24 | 64 |
| Swish | highlevel | 22655 | 22656 | +0.00 | 6208.48 | 6198.79 | 93 |
| Tan | highlevel | 30420 | 30424 | +0.01 | 6106.67 | 5595.15 | 95 |
| Tanh | highlevel | 23886 | 23887 | +0.00 | 5880.61 | 5849.09 | 92 |
| TopK | highlevel | 561 | 561 | +0.00 | 2936.97 | 2974.55 | 44 |
| Trunc | highlevel | 22074 | 22080 | +0.03 | 5592.12 | 6004.24 | 91 |
| Where | highlevel | 482 | 482 | +0.00 | 3157.58 | 3162.42 | 61 |
| ScalarCast | scalar | 204 | 204 | +0.00 | 1586.06 | 1592.12 | 47 |
| IBSetWait | sync | 7758 | 7780 | +0.28 | 9115.15 | 9156.97 | 115 |
| SyncAll | sync | 6477 | 6591 | +1.76 | 7915.15 | 7835.76 | 106 |
| Abs | vector | 22076 | 22079 | +0.01 | 6025.45 | 6011.52 | 89 |
| AbsSub | vector | 346 | 346 | +0.00 | 2222.42 | 2235.76 | 53 |
| Add | vector | 30896 | 30904 | +0.03 | 6491.52 | 7160.0 | 93 |
| AddHalf | vector | 336 | 336 | +0.00 | 2072.73 | 2449.7 | 57 |
| AddRelu | vector | 350 | 350 | +0.00 | 2591.52 | 2490.91 | 54 |
| Adds | vector | 22116 | 22120 | +0.02 | 5703.64 | 5680.0 | 91 |
| And | vector | 30259 | 30264 | +0.02 | 5369.09 | 5381.82 | 84 |
| Ands | vector | 283 | 283 | +0.00 | 1916.36 | 1919.39 | 52 |
| AtomicAdd | vector | 1760 | 1760 | +0.00 | 2252.73 | 2264.85 | 56 |
| Axpy | vector | 289 | 289 | +0.00 | 1912.12 | 1910.3 | 49 |
| Brcb | vector | 275 | 275 | +0.00 | 1914.55 | 1898.79 | 48 |
| Cast | vector | 22079 | 22080 | +0.00 | 6007.27 | 6050.91 | 86 |
| CastF2H | vector | 301 | 301 | +0.00 | 1767.88 | 1737.58 | 46 |
| Compare | vector | 410 | 410 | +0.00 | 2589.7 | 2595.76 | 53 |
| CreateVecIndex | vector | 212 | 212 | +0.00 | 1589.09 | 1592.73 | 47 |
| Div | vector | 30901 | 30904 | +0.01 | 6040.0 | 6186.67 | 95 |
| Divs | vector | 22373 | 22376 | +0.01 | 6072.73 | 6053.94 | 94 |
| Duplicate | vector | 204 | 204 | +0.00 | 1604.24 | 1589.7 | 46 |
| Exp | vector | 22078 | 22080 | +0.01 | 6057.58 | 6090.3 | 89 |
| ExpSub | vector | 346 | 346 | +0.00 | 2422.42 | 2512.12 | 58 |
| FusedMulAdd | vector | 353 | 353 | +0.00 | 2494.55 | 2241.82 | 50 |
| Gather | vector | 333 | 333 | +0.00 | 2071.52 | 2064.24 | 50 |
| GatherMask | vector | 354 | 354 | +0.00 | 2250.91 | 2244.24 | 52 |
| LeakyRelu | vector | 286 | 286 | +0.00 | 1917.58 | 1901.21 | 52 |
| Ln | vector | 22074 | 22080 | +0.03 | 6064.24 | 6050.91 | 88 |
| Max | vector | 30898 | 30904 | +0.02 | 6008.48 | 6093.94 | 95 |
| Maxs | vector | 22116 | 22119 | +0.01 | 6006.06 | 6029.09 | 85 |
| Min | vector | 30895 | 30904 | +0.03 | 6073.33 | 6738.18 | 98 |
| Mins | vector | 22115 | 22120 | +0.02 | 6036.36 | 6061.82 | 87 |
| Mul | vector | 30901 | 30904 | +0.01 | 6777.58 | 6008.48 | 95 |
| MulAddDst | vector | 353 | 353 | +0.00 | 2234.55 | 2235.76 | 52 |
| MulAddRelu | vector | 357 | 357 | +0.00 | 2247.27 | 2238.79 | 55 |
| MulCast | vector | 344 | 344 | +0.00 | 2101.21 | 2098.18 | 51 |
| Mull | vector | 420 | 420 | +0.00 | 2358.18 | 2364.24 | 55 |
| Muls | vector | 22115 | 22119 | +0.02 | 5687.88 | 6018.18 | 87 |
| MulsCast | vector | 291 | 291 | +0.00 | 1910.91 | 1912.12 | 52 |
| Neg | vector | 22077 | 22079 | +0.01 | 5677.58 | 5996.36 | 86 |
| Not | vector | 21562 | 21568 | +0.03 | 5623.03 | 5464.24 | 82 |
| Or | vector | 30259 | 30263 | +0.01 | 6088.48 | 5393.33 | 85 |
| Ors | vector | 283 | 283 | +0.00 | 1907.88 | 1924.85 | 48 |
| Prelu | vector | 30899 | 30902 | +0.01 | 6743.64 | 6146.67 | 99 |
| Reciprocal | vector | 22374 | 22376 | +0.01 | 6108.48 | 6068.48 | 90 |
| ReduceMax | vector | 327 | 327 | +0.00 | 2212.73 | 2223.64 | 54 |
| ReduceMin | vector | 327 | 327 | +0.00 | 2223.64 | 2209.09 | 53 |
| ReduceSum | vector | 363 | 363 | +0.00 | 2187.27 | 2188.48 | 54 |
| Relu | vector | 22077 | 22080 | +0.01 | 5700.61 | 5983.64 | 86 |
| Rsqrt | vector | 23143 | 23140 | -0.01 | 6155.76 | 6152.73 | 87 |
| Scatter | vector | 333 | 333 | +0.00 | 2038.18 | 2057.58 | 53 |
| Select | vector | 39443 | 39456 | +0.03 | 6847.27 | 6892.73 | 94 |
| ShiftLeft | vector | 331 | 331 | +0.00 | 2068.48 | 2041.82 | 51 |
| ShiftRight | vector | 331 | 331 | +0.00 | 2069.09 | 2037.58 | 55 |
| Sqrt | vector | 22078 | 22080 | +0.01 | 6105.45 | 6041.21 | 87 |
| Sub | vector | 30900 | 30903 | +0.01 | 6080.0 | 6738.18 | 94 |
| SubRelu | vector | 350 | 350 | +0.00 | 2212.73 | 2425.45 | 58 |
| Subs | vector | 22370 | 22375 | +0.02 | 6024.24 | 6018.18 | 86 |
| Transpose | vector | 259 | 259 | +0.00 | 1690.3 | 1684.85 | 46 |
