# CANN API Explorer — Performance Analysis Report

> 中文版: [PERF.zh-CN.md](PERF.zh-CN.md)
>
> Data source: the `record.log` of 84 units run under cannsim/CAModel cardless simulation, taking two **deterministic hardware metrics**:
> - **Instruction count** (`Total number of instruction executions`): the number of AI Core instructions actually executed on device.
> - **Time ns** (`Execution time (ns)`): the **hardware execution time estimate computed** by CAModel via instruction-level simulation.
>
> **Methodology note (important)**: these two metrics are **deterministic** — for the same kernel + input, the results are identical regardless of whether the host machine is busy. Host load only affects `sim time / speed` in `cannsim.log` (the **simulator's own wall-clock**); it **does not affect the two simulation results above**. The data in this report comes from a full re-run while the machine was idle, and matches the earlier values measured while busy, which confirms this point. (The Cycles table at line 94 of `record.log` is cannsim's built-in **typical-operator reference baselines** for FA/Matmul etc., not the results of this project's units, and is not used.)

## 1. Instruction count distribution (overview)

| Magnitude | Representative operators | instr | Interpretation |
|------|---------|-------|------|
| **100k-level** | Lgamma | **159997** | log-gamma function, polynomial/series approximation, instruction explosion (far beyond the rest) |
| **30k–40k** | Atan(43113)/Power(41654)/Hypot(40110)/Select(39443)/Acos(37187)/Fmod(36540)/Asin(36044)/Erfc(33660)/Cos(31848) | 30k–40k | transcendental functions (inverse-trig / exp-class) and binary math, many approximation expansion terms; Select includes Compare + mask |
| **20k-level** | activations (Gelu 25135/Sigmoid 22911/Silu/Swish ~22.7k), most unary math | ~20k | element-wise compute over 8 cores × 2048 elements |
| **10k–20k** | vector binary/unary/reduce, DataCopy(19005) | 10k–20k | basic vector ops (fully loaded 8 cores × 2048) |
| **Hundreds-level** | hand-written single-core small operators, tiling normalization | 200–900 | see below |
| **Most economical TOP** | ScalarCast(204)/Duplicate(204)/CreateVecIndex(212)/Transpose(259)/Brcb(275)/Axpy(289)/CastF2H(301)/Pad(318) | <320 | single core + small data + single/few vector instructions |

## 2. Time (execution time ns) TOP

| Rank | Operator | ns | instr | Why the time is high |
|------|------|-----|-------|-----------|
| 1 | Lgamma | 9673 | 159997 | largest absolute instruction count |
| 2 | **Matmul** | 9556 | **only 5245** | **Cube path**: heavy MMAD instructions + L1/L0 data movement + deep pipeline, per-instruction cost far higher than vector |
| 3 | **IBSetWait** | 9115 | 7758 | **multi-core sync**: inter-core busy-wait polling (while-polling a GM flag) generates a large number of wait instructions |
| 4 | **SyncAll** | 7915 | 6477 | multi-core full barrier, software-barrier polling overhead |
| 5+ | Atan/Select/Power/Fmod | ~6800 | 30k–40k | instruction-intensive |

## 3. time/instr ratio — fixed overhead vs arithmetic-bound

| Ratio | Operator | Meaning |
|------|------|------|
| **High (6–8 ns/instr)** | Duplicate(7.86)/ScalarCast(7.78)/CreateVecIndex(7.50)/Pad(7.28)/Brcb/ReduceMin/Axpy | **small operators dominated by fixed overhead**: the fixed cost of kernel launch + DataCopy movement, amortized over only ~200–300 instructions, gives a high per-instruction average |
| Medium (~1.8) | Matmul(1.82) | the Cube single instruction is heavy in itself |
| **Low (0.06–0.17)** | Lgamma(0.06)/Hypot/Atan/Power/Acos | **large operators dominated by arithmetic instructions**: massive lightweight scalar/vector arithmetic dilutes the fixed overhead |

> Insight: the smaller the operator, the **larger the share of fixed overhead from data movement and launch** (which shows up as a high time/instr under CAModel); the larger the operator, the closer it gets to pure arithmetic throughput. To optimize small operators, reduce movement / merge kernels; to optimize large operators, reduce instruction count (algorithm / approximation order).

## 4. Tiling / Cube insights

| Operator | instr | ns | Note |
|------|-------|-----|------|
| SoftMax | 423 | 2189 | device-side TilingFunc |
| LogSoftmax | 514 | 2608 | reuses SoftMaxTiling |
| RmsNorm | 553 | 2425 | hand-filled tiling |
| TopK | 561 | 2937 | host tiling |
| Broadcast | 620 | 2485 | device computes tiling |
| GroupNorm | 634 | 2501 | hand-filled |
| DeepNorm/BatchNorm | 854/863 | ~3300 | hand-filled, includes residual / along-axis reduction |
| LayerNorm | 909 | 2767 | regbase |
| Sort | 1774 | 3894 | merge sort |
| **Matmul** | **5245** | **9556** | **Cube, magnitude far above the vector-class tiling operators** |

> Key conclusion: **normalization/sort operators with tiling have relatively few device-side instructions (hundreds-level)** — their "complexity" lies mainly in the **host-side tiling computation** (which produces no device instructions and is not counted here). What is truly expensive on device is **Cube (Matmul)**: medium instruction count, but because of MMAD + movement + pipeline, the time is **3–4×** that of the vector-class tiling operators.

## 5. Multi-core sync overhead

| Operator | instr | ns | Description |
|------|-------|-----|------|
| AtomicAdd | 1760 | 2253 | 8-core atomic accumulation, moderate overhead |
| SyncAll | 6477 | 7915 | full barrier, software polling |
| IBSetWait | 7758 | 9115 | chained one-to-one, **busy-wait polling is heaviest** |

> Multi-core sync (SyncAll/IBSet/IBWait) has both instruction count and time significantly higher than single-core compute — **the polling of inter-core waits (while-polling a GM flag) is the main cost**, which is the inherent price of deterministic synchronization primitives.

## 6. Intra-family comparison

- **Activation family** (8 cores × 2048, simple count pattern): Silu(22652) ≈ Swish(22655) < Sigmoid(22911) < Gelu(25135); Gelu is slightly more expensive because it contains an erf/tanh approximation.
- **Normalization family** (device instructions, by complexity): SoftMax(423) < RmsNorm(553) < GroupNorm(634) < DeepNorm/BatchNorm(~860) < LayerNorm(909) — increasing as the reduction dimension and affine terms grow.
- **Data-rearrangement family**: all 200–354 instr, the lightest (Duplicate/CreateVecIndex only generate; ScalarCast/Duplicate, with no input movement, are the most economical).

## Per-category averages

| Category | Units | Avg instr | Avg ns |
|------|--------|-----------|---------|
| scalar | 1 | 204 | 1586 |
| vector | 41 | 15771 | 4478 |
| highlevel | 38 | 23630 | 5317 |
| sync | 2 | 7118 | 8515 |
| cube | 1 | 5245 | 9556 |
| datacopy | 1 | 19005 | 5884 |

> highlevel has the highest average instr (pulled up by transcendental functions such as Lgamma); sync/cube have the highest average ns (sync polling / Cube pipeline).
