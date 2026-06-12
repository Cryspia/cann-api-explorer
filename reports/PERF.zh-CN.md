# CANN API Explorer — 性能分析报告

> English: [PERF.md](PERF.md)
>
> 数据来源：84 个单元在 cannsim/CAModel 无卡仿真下的 `record.log`，取两个**确定性硬件指标**：
> - **指令执行数**（`Total number of instruction executions`）：device 实际执行的 AI Core 指令条数。
> - **执行时间 ns**（`Execution time (ns)`）：CAModel 按指令级仿真**算出的硬件执行时间估计**。
>
> **方法论要点（重要）**：这两个指标是**确定性的**——同样的 kernel + 输入，无论宿主机是否繁忙，结果完全一致。宿主机负载只影响 `cannsim.log` 里的 `sim time / speed`（**仿真器自身的墙钟**），**不影响上面两个仿真结果**。本报告数据来自机器空闲时的全量重跑，与之前繁忙时的值一致，印证了这一点。（`record.log` 里 line 94 的 Cycles 表是 cannsim 内置的 FA/Matmul 等**典型算子参考基准**，非本项目单元结果，未采用。）

## 1. 指令数分布（全景）

| 量级 | 代表算子 | instr | 解读 |
|------|---------|-------|------|
| **10 万级** | Lgamma | **159997** | 对数伽马函数，多项式/级数逼近，指令爆炸（远超其他） |
| **3–4 万** | Atan(43113)/Power(41654)/Hypot(40110)/Select(39443)/Acos(37187)/Fmod(36540)/Asin(36044)/Erfc(33660)/Cos(31848) | 3–4w | 超越函数(反三角/exp 类)与二元数学，逼近展开项多；Select 含 Compare+掩码 |
| **2 万级** | 激活(Gelu 25135/Sigmoid 22911/Silu/Swish ~22.7k)、多数一元数学 | ~2w | 8 核 × 2048 元素的逐元素计算 |
| **1–2 万** | 矢量 binary/unary/reduce、DataCopy(19005) | 1–2w | 基础矢量 op（满载 8 核 2048） |
| **数百级** | 手写单核小算子、Tiling 归一化 | 200–900 | 见下 |
| **最省 TOP** | ScalarCast(204)/Duplicate(204)/CreateVecIndex(212)/Transpose(259)/Brcb(275)/Axpy(289)/CastF2H(301)/Pad(318) | <320 | 单核 + 小数据 + 单条/少量向量指令 |

## 2. 耗时（执行时间 ns）TOP

| 排名 | 算子 | ns | instr | 为何耗时高 |
|------|------|-----|-------|-----------|
| 1 | Lgamma | 9673 | 159997 | 指令绝对数最多 |
| 2 | **Matmul** | 9556 | **仅 5245** | **Cube 路径**：MMAD 指令重 + L1/L0 数据搬运 + 流水深，单指令成本远高于矢量 |
| 3 | **IBSetWait** | 9115 | 7758 | **多核同步**：核间轮询忙等（while 轮询 GM flag）产生大量等待指令 |
| 4 | **SyncAll** | 7915 | 6477 | 多核全屏障，软件屏障轮询开销 |
| 5+ | Atan/Select/Power/Fmod | ~6800 | 3–4w | 指令密集 |

## 3. time/instr 比 —— 固定开销 vs 算术密集

| 比值 | 算子 | 含义 |
|------|------|------|
| **高（6–8 ns/instr）** | Duplicate(7.86)/ScalarCast(7.78)/CreateVecIndex(7.50)/Pad(7.28)/Brcb/ReduceMin/Axpy | **小算子被固定开销主导**：kernel 启动 + DataCopy 搬运的固定成本，摊到仅 ~200–300 条指令上，单指令均摊高 |
| 中（~1.8） | Matmul(1.82) | Cube 单指令本身重 |
| **低（0.06–0.17）** | Lgamma(0.06)/Hypot/Atan/Power/Acos | **大算子被算术指令主导**：海量轻量标量/向量算术，固定开销被稀释 |

> 洞察：算子越小，**数据搬运与启动的固定开销占比越大**（CAModel 下表现为 time/instr 高）；算子越大，越接近纯算术吞吐。优化小算子要减搬运/合并 kernel，优化大算子要减指令数（算法/逼近阶数）。

## 4. Tiling / Cube 洞察

| 算子 | instr | ns | 备注 |
|------|-------|-----|------|
| SoftMax | 423 | 2189 | device 端 TilingFunc |
| LogSoftmax | 514 | 2608 | 复用 SoftMaxTiling |
| RmsNorm | 553 | 2425 | 手填 tiling |
| TopK | 561 | 2937 | host tiling |
| Broadcast | 620 | 2485 | device 算 tiling |
| GroupNorm | 634 | 2501 | 手填 |
| DeepNorm/BatchNorm | 854/863 | ~3300 | 手填，含残差/沿轴归约 |
| LayerNorm | 909 | 2767 | regbase |
| Sort | 1774 | 3894 | 归并排序 |
| **Matmul** | **5245** | **9556** | **Cube，量级远超矢量类 Tiling** |

> 关键结论：**带 Tiling 的归一化/排序算子，device 端指令并不多（数百级）**——它们的"复杂度"主要在 **host 侧 tiling 计算**（不产生 device 指令、不计入此处）。真正在 device 上贵的是 **Cube(Matmul)**：指令数中等但因 MMAD + 搬运 + 流水，耗时是矢量类 Tiling 算子的 **3–4 倍**。

## 5. 多核同步开销

| 算子 | instr | ns | 说明 |
|------|-------|-----|------|
| AtomicAdd | 1760 | 2253 | 8 核原子累加，开销适中 |
| SyncAll | 6477 | 7915 | 全屏障，软件轮询 |
| IBSetWait | 7758 | 9115 | 链式一对一，**轮询忙等最重** |

> 多核同步（SyncAll/IBSet/IBWait）的指令与耗时都显著高于单核计算——**核间等待的轮询（while 轮询 GM flag）是主要成本**，这是确定性同步原语的固有代价。

## 6. 同族横向对比

- **激活家族**（8 核 2048，简单 count 模式）：Silu(22652) ≈ Swish(22655) < Sigmoid(22911) < Gelu(25135)，Gelu 因含 erf/tanh 近似略贵。
- **归一化家族**（device 指令，按复杂度）：SoftMax(423) < RmsNorm(553) < GroupNorm(634) < DeepNorm/BatchNorm(~860) < LayerNorm(909)——随归约维度与仿射项增多递增。
- **数据重排家族**：全部 200–354 instr，最轻量（Duplicate/CreateVecIndex 仅生成，无输入搬运的 ScalarCast/Duplicate 最省）。

## 各类别平均

| 类别 | 单元数 | 平均 instr | 平均 ns |
|------|--------|-----------|---------|
| scalar | 1 | 204 | 1586 |
| vector | 41 | 15771 | 4478 |
| highlevel | 38 | 23630 | 5317 |
| sync | 2 | 7118 | 8515 |
| cube | 1 | 5245 | 9556 |
| datacopy | 1 | 19005 | 5884 |

> highlevel 平均 instr 最高（含 Lgamma 等超越函数拉高）；sync/cube 平均 ns 最高（同步轮询 / Cube 流水）。
