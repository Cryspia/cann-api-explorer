# CANN API Explorer — Performance Analysis Report

> 中文版: [PERF.zh-CN.md](PERF.zh-CN.md)
>
> Data source: the `record.log` of 172 units run under cannsim/CAModel cardless simulation, taking two **deterministic hardware metrics**:
> - **Instruction count** (`Total number of instruction executions`): the number of AI Core instructions actually executed on device.
> - **Time ns** (`Execution time (ns)`): the **hardware execution time estimate computed** by CAModel via instruction-level simulation.
>
> **Methodology note (important)**: these two metrics are **deterministic** — for the same kernel + input, the results are identical regardless of whether the host machine is busy. Host load only affects `sim time / speed` in `cannsim.log` (the **simulator's own wall-clock**); it **does not affect the two simulation results above**. This was reconfirmed this round: the full 172-unit suite was re-run end-to-end on a second machine (aarch64) and the instruction/time values match this machine's to within ~1 instruction (e.g. DataCopy 19005 vs 19006). (The Cycles table at line 94 of `record.log` is cannsim's built-in **typical-operator reference baselines** for FA/Matmul etc., not the results of this project's units, and is not used.)

## 1. Instruction count distribution (overview)

| Magnitude | Representative operators | instr | Interpretation |
|------|---------|-------|------|
| **100k-level** | Lgamma | **159993** | log-gamma function, polynomial/series approximation, instruction explosion (far beyond the rest) |
| **70k-level** | Digamma | **77175** | digamma (ψ) function, same gamma-family series expansion — the second instruction explosion |
| **30k–45k** | Atan(43117)/Power(41657)/Hypot(40102)/Select(39450)/Acos(37188)/Fmod(36540)/Asin(36045)/Erfc(33656)/LogicalOr(33414)/LogicalXor(33411) | 30k–45k | transcendental functions (inverse-trig / exp-class), binary math, and bool-output logical ops; Select includes Compare + mask |
| **20k-level** | activations (Gelu 25133/Sigmoid 22907/Silu 22655/Swish 22654), most unary math, vector binary (Mul/Prelu ~30.9k) | ~20k–31k | element-wise compute over 8 cores × 2048 elements |
| **10k–20k** | vector binary/unary/reduce, DataCopy(19006) | 10k–20k | basic vector ops (fully loaded 8 cores × 2048) |
| **Hundreds-level** | hand-written single-core small operators, tiling normalization, reduce-granularity / rearrange / quant variants | 200–1800 | see below |
| **Most economical TOP** | ScalarCast(204)/Duplicate(204)/CreateVecIndex(212)/ArithProgression(223)/Transpose(259)/BitwiseNot(274)/Brcb(275)/Truncate(281)/Ands(283)/Ors(283) | <290 | single core + small data + single/few vector instructions |

## 2. Time (execution time ns) TOP

| Rank | Operator | ns | instr | Why the time is high |
|------|------|-----|-------|-----------|
| 1 | **Digamma** | 9701 | 77175 | gamma-family series approximation; now the highest absolute time |
| 2 | Lgamma | 9687 | **159993** | largest absolute instruction count |
| 3 | **Matmul** | 9539 | **only 5245** | **Cube path**: heavy MMAD instructions + L1/L0 data movement + deep pipeline, per-instruction cost far higher than vector |
| 4 | **IBSetWait** | 9129 | 7780 | **multi-core sync**: inter-core busy-wait polling (while-polling a GM flag) generates a large number of wait instructions |
| 5 | **SyncAll** | 7255 | 6256 | multi-core full barrier, software-barrier polling overhead |
| 6+ | Prelu/Power/Mul/Fmod/Atan/Select/Hypot | ~6800–7200 | 30k–43k | instruction-intensive |

## 3. time/instr ratio — fixed overhead vs arithmetic-bound

| Ratio | Operator | Meaning |
|------|------|------|
| **High (7–9 ns/instr)** | ArithProgression(8.94)/AscendAntiQuant(8.37)/ScalarCast(7.81)/Interleave(7.79)/AscendDequant(7.72)/Duplicate(7.66)/ReduceAll(7.60)/CreateVecIndex(7.57)/AscendQuant(7.53) | **small operators dominated by fixed overhead**: the fixed cost of kernel launch + DataCopy movement, amortized over only ~200–360 instructions, gives a high per-instruction average |
| Medium (~1.8) | Matmul(1.82) | the Cube single instruction is heavy in itself |
| **Low (0.06–0.17)** | Lgamma(0.061)/Digamma(0.126)/Acos(0.157)/Atan(0.160)/Hypot(0.170)/Power(0.170) | **large operators dominated by arithmetic instructions**: massive lightweight scalar/vector arithmetic dilutes the fixed overhead |

> Insight: the smaller the operator, the **larger the share of fixed overhead from data movement and launch** (which shows up as a high time/instr under CAModel); the larger the operator, the closer it gets to pure arithmetic throughput. To optimize small operators, reduce movement / merge kernels; to optimize large operators, reduce instruction count (algorithm / approximation order).

## 4. Tiling / Cube insights

| Operator | instr | ns | Note |
|------|-------|-----|------|
| SoftMax | 423 | 2188 | device-side TilingFunc |
| WelfordFinalize | 460 | 2666 | combine partial mean/variance |
| SimpleSoftMax | 523 | 2676 | softmax given precomputed max/sum |
| RmsNorm | 553 | 2438 | hand-filled tiling |
| TopK | 561 | 2927 | host tiling |
| Broadcast | 620 | 2484 | device computes tiling |
| GroupNorm | 634 | 2505 | hand-filled |
| SoftmaxFlashV2 | 648 | 2777 | flash-attention online softmax |
| DeepNorm/BatchNorm | 854/863 | ~3300–3500 | hand-filled, includes residual / along-axis reduction |
| LayerNorm | 909 | 2763 | regbase |
| Normalize | 916 | 3411 | LayerNorm second half, given mean/var |
| SoftmaxFlashV3 | 1386 | 3621 | flash v3, extra mean/shift terms (heaviest of the softmax family) |
| Sort | 1774 | 3888 | merge sort |
| **Matmul** | **5245** | **9539** | **Cube, magnitude far above the vector-class tiling operators** |

> Key conclusion: **normalization/sort/softmax operators with tiling have relatively few device-side instructions (hundreds- to ~1.8k-level)** — their "complexity" lies mainly in the **host-side tiling computation** (which produces no device instructions and is not counted here). What is truly expensive on device is **Cube (Matmul)**: medium instruction count, but because of MMAD + movement + pipeline, the time is **3–4×** that of the vector-class tiling operators.

## 5. Multi-core sync overhead

| Operator | instr | ns | Description |
|------|-------|-----|------|
| AtomicAdd | 1760 | 2243 | 8-core atomic accumulation, moderate overhead |
| SyncAll | 6256 | 7255 | full barrier, software polling |
| IBSetWait | 7780 | 9129 | chained one-to-one, **busy-wait polling is heaviest** |

> Multi-core sync (SyncAll/IBSet/IBWait) has both instruction count and time significantly higher than single-core compute — **the polling of inter-core waits (while-polling a GM flag) is the main cost**, which is the inherent price of deterministic synchronization primitives.

## 6. Intra-family comparison

- **Activation family** (8 cores × 2048, simple count pattern): Swish(22654) ≈ Silu(22655) < Sigmoid(22907) < Gelu(25133); Gelu is slightly more expensive because it contains an erf/tanh approximation.
- **Normalization family** (device instructions, by complexity): SoftMax(423) < RmsNorm(553) < GroupNorm(634) < DeepNorm(854) < BatchNorm(863) < LayerNorm(909) < Normalize(916) — increasing as the reduction dimension and affine terms grow.
- **Softmax family**: SoftMax(423) < SimpleSoftMax(523) < SoftmaxFlashV2(648) < SoftmaxFlashV3(1386) — the flash variants carry running max/sum (and v3 a mean/shift), so cost grows with the online-statistics bookkeeping.
- **Gamma family**: Digamma(77175) and Lgamma(159993) are the two instruction-explosion outliers — series/polynomial approximations of ψ and lnΓ dominate the entire suite.
- **Reduce-granularity family** (vector): block/whole/pair/repeat reduce variants land in the hundreds-to-low-thousands range; their per-instruction time is high (fixed-overhead-bound) like the data-rearrangement family.
- **Data-rearrangement family**: Duplicate/CreateVecIndex/Brcb/Interleave/ScalarCast all 200–360 instr, the lightest (generate-only or no input movement → the most economical).

## Per-category averages

| Category | Units | Avg instr | Avg ns |
|------|--------|-----------|---------|
| scalar | 1 | 204 | 1594 |
| vector | 73 | 9425 | 3549 |
| highlevel | 94 | 14394 | 4204 |
| sync | 2 | 7018 | 8192 |
| cube | 1 | 5245 | 9539 |
| datacopy | 1 | 19006 | 5858 |

> highlevel has the highest average instr (pulled up by transcendental / gamma functions such as Lgamma and Digamma); sync/cube have the highest average ns (sync polling / Cube pipeline).
