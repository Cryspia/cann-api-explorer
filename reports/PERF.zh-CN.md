# CANN API Explorer — 性能分析报告

> English: [PERF.md](PERF.md)
>
> 数据来源：172 个单元在 cannsim/CAModel 无卡仿真下的 `record.log`，取两个**确定性硬件指标**：
> - **指令执行数**（`Total number of instruction executions`）：device 实际执行的 AI Core 指令条数。
> - **执行时间 ns**（`Execution time (ns)`）：CAModel 按指令级仿真**算出的硬件执行时间估计**。
>
> **方法论要点（重要）**：这两个指标是**确定性的**——同样的 kernel + 输入，无论宿主机是否繁忙，结果完全一致。宿主机负载只影响 `cannsim.log` 里的 `sim time / speed`（**仿真器自身的墙钟**），**不影响上面两个仿真结果**。本轮再次印证：全量 172 单元在第二台机器（aarch64）端到端重跑，指令/耗时与本机相差在 ~1 条指令内（如 DataCopy 19005 vs 19006）。（`record.log` 里 line 94 的 Cycles 表是 cannsim 内置的 FA/Matmul 等**典型算子参考基准**，非本项目单元结果，未采用。）

## 1. 指令数分布（全景）

| 量级 | 代表算子 | instr | 解读 |
|------|---------|-------|------|
| **10 万级** | Lgamma | **159993** | 对数伽马函数，多项式/级数逼近，指令爆炸（远超其他） |
| **7 万级** | Digamma | **77175** | 双伽马(ψ)函数，同属伽马家族级数展开——第二个指令爆炸点 |
| **3–4.5 万** | Atan(43117)/Power(41657)/Hypot(40102)/Select(39450)/Acos(37188)/Fmod(36540)/Asin(36045)/Erfc(33656)/LogicalOr(33414)/LogicalXor(33411) | 3–4.5w | 超越函数(反三角/exp 类)、二元数学、bool 输出逻辑；Select 含 Compare+掩码 |
| **2–3 万** | 激活(Gelu 25133/Sigmoid 22907/Silu 22655/Swish 22654)、多数一元数学、矢量二元(Mul/Prelu ~30.9k) | ~2–3.1w | 8 核 × 2048 元素的逐元素计算 |
| **1–2 万** | 矢量 binary/unary/reduce、DataCopy(19006) | 1–2w | 基础矢量 op（满载 8 核 2048） |
| **数百级** | 手写单核小算子、Tiling 归一化、归约粒度/重排/量化变体 | 200–1800 | 见下 |
| **最省 TOP** | ScalarCast(204)/Duplicate(204)/CreateVecIndex(212)/ArithProgression(223)/Transpose(259)/BitwiseNot(274)/Brcb(275)/Truncate(281)/Ands(283)/Ors(283) | <290 | 单核 + 小数据 + 单条/少量向量指令 |

## 2. 耗时（执行时间 ns）TOP

| 排名 | 算子 | ns | instr | 为何耗时高 |
|------|------|-----|-------|-----------|
| 1 | **Digamma** | 9701 | 77175 | 伽马家族级数逼近；现绝对耗时最高 |
| 2 | Lgamma | 9687 | **159993** | 指令绝对数最多 |
| 3 | **Matmul** | 9539 | **仅 5245** | **Cube 路径**：MMAD 指令重 + L1/L0 数据搬运 + 流水深，单指令成本远高于矢量 |
| 4 | **IBSetWait** | 9129 | 7780 | **多核同步**：核间轮询忙等（while 轮询 GM flag）产生大量等待指令 |
| 5 | **SyncAll** | 7255 | 6256 | 多核全屏障，软件屏障轮询开销 |
| 6+ | Prelu/Power/Mul/Fmod/Atan/Select/Hypot | ~6800–7200 | 3–4.3w | 指令密集 |

## 3. time/instr 比 —— 固定开销 vs 算术密集

| 比值 | 算子 | 含义 |
|------|------|------|
| **高（7–9 ns/instr）** | ArithProgression(8.94)/AscendAntiQuant(8.37)/ScalarCast(7.81)/Interleave(7.79)/AscendDequant(7.72)/Duplicate(7.66)/ReduceAll(7.60)/CreateVecIndex(7.57)/AscendQuant(7.53) | **小算子被固定开销主导**：kernel 启动 + DataCopy 搬运的固定成本，摊到仅 ~200–360 条指令上，单指令均摊高 |
| 中（~1.8） | Matmul(1.82) | Cube 单指令本身重 |
| **低（0.06–0.17）** | Lgamma(0.061)/Digamma(0.126)/Acos(0.157)/Atan(0.160)/Hypot(0.170)/Power(0.170) | **大算子被算术指令主导**：海量轻量标量/向量算术，固定开销被稀释 |

> 洞察：算子越小，**数据搬运与启动的固定开销占比越大**（CAModel 下表现为 time/instr 高）；算子越大，越接近纯算术吞吐。优化小算子要减搬运/合并 kernel，优化大算子要减指令数（算法/逼近阶数）。

## 4. Tiling / Cube 洞察

| 算子 | instr | ns | 备注 |
|------|-------|-----|------|
| SoftMax | 423 | 2188 | device 端 TilingFunc |
| WelfordFinalize | 460 | 2666 | 合并分块均值/方差 |
| SimpleSoftMax | 523 | 2676 | 给定预算好的 max/sum 的 softmax |
| RmsNorm | 553 | 2438 | 手填 tiling |
| TopK | 561 | 2927 | host tiling |
| Broadcast | 620 | 2484 | device 算 tiling |
| GroupNorm | 634 | 2505 | 手填 |
| SoftmaxFlashV2 | 648 | 2777 | flash-attention 在线 softmax |
| DeepNorm/BatchNorm | 854/863 | ~3300–3500 | 手填，含残差/沿轴归约 |
| LayerNorm | 909 | 2763 | regbase |
| Normalize | 916 | 3411 | LayerNorm 后半段，给定 mean/var |
| SoftmaxFlashV3 | 1386 | 3621 | flash v3，多 mean/shift 项（softmax 家族最重） |
| Sort | 1774 | 3888 | 归并排序 |
| **Matmul** | **5245** | **9539** | **Cube，量级远超矢量类 Tiling** |

> 关键结论：**带 Tiling 的归一化/排序/softmax 算子，device 端指令并不多（数百到 ~1.8k 级）**——它们的"复杂度"主要在 **host 侧 tiling 计算**（不产生 device 指令、不计入此处）。真正在 device 上贵的是 **Cube(Matmul)**：指令数中等但因 MMAD + 搬运 + 流水，耗时是矢量类 Tiling 算子的 **3–4 倍**。

## 5. 多核同步开销

| 算子 | instr | ns | 说明 |
|------|-------|-----|------|
| AtomicAdd | 1760 | 2243 | 8 核原子累加，开销适中 |
| SyncAll | 6256 | 7255 | 全屏障，软件轮询 |
| IBSetWait | 7780 | 9129 | 链式一对一，**轮询忙等最重** |

> 多核同步（SyncAll/IBSet/IBWait）的指令与耗时都显著高于单核计算——**核间等待的轮询（while 轮询 GM flag）是主要成本**，这是确定性同步原语的固有代价。

## 6. 同族横向对比

- **激活家族**（8 核 2048，简单 count 模式）：Swish(22654) ≈ Silu(22655) < Sigmoid(22907) < Gelu(25133)，Gelu 因含 erf/tanh 近似略贵。
- **归一化家族**（device 指令，按复杂度）：SoftMax(423) < RmsNorm(553) < GroupNorm(634) < DeepNorm(854) < BatchNorm(863) < LayerNorm(909) < Normalize(916)——随归约维度与仿射项增多递增。
- **Softmax 家族**：SoftMax(423) < SimpleSoftMax(523) < SoftmaxFlashV2(648) < SoftmaxFlashV3(1386)——flash 变体带 running max/sum（v3 还多 mean/shift），随在线统计记账递增。
- **伽马家族**：Digamma(77175) 与 Lgamma(159993) 是两个指令爆炸离群点——ψ 与 lnΓ 的级数/多项式逼近主导整个套件。
- **归约粒度家族**（矢量）：block/whole/pair/repeat 归约变体落在数百到低千区间；单指令耗时高（固定开销主导），同数据重排家族。
- **数据重排家族**：Duplicate/CreateVecIndex/Brcb/Interleave/ScalarCast 全部 200–360 instr，最轻量（仅生成或无输入搬运 → 最省）。

## 各类别平均

| 类别 | 单元数 | 平均 instr | 平均 ns |
|------|--------|-----------|---------|
| scalar | 1 | 204 | 1594 |
| vector | 73 | 9425 | 3549 |
| highlevel | 94 | 14394 | 4204 |
| sync | 2 | 7018 | 8192 |
| cube | 1 | 5245 | 9539 |
| datacopy | 1 | 19006 | 5858 |

> highlevel 平均 instr 最高（含 Lgamma、Digamma 等超越/伽马函数拉高）；sync/cube 平均 ns 最高（同步轮询 / Cube 流水）。
