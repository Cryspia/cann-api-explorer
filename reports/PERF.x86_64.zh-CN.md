# CANN API Explorer — x86_64 验证与跨架构对比

> English: [PERF.x86_64.md](PERF.x86_64.md)
>
> 本报告记录把项目在 **x86_64** 上跑通并与已提交的 **aarch64** 基线对比的过程。aarch64 的结果（[`INDEX.md`](INDEX.md) / [`PERF.md`](PERF.md)）保持原样不被覆盖；x86 结果存放在带架构后缀的文件里（[`INDEX.x86_64.md`](INDEX.x86_64.md)、各单元 `RESULT.x86_64.md`）。

## 结论：**x86_64 上 172/172 仿真全部通过** ✅

- **本机**：AMD Ryzen AI MAX+ 395，16 核 / 31 GiB 内存（Linux x86_64）——恰好踩在 CAModel 的最低配置（≥16 核、≥32 GB）。
- **安装包**：`Ascend-cann_9.1.0-beta.1_linux-x86_64.run`，仅 toolkit（无 NPU 驱动），纯无卡 CAModel 仿真。Build 芯片 Ascend950PR_9599 / Run 芯片 Ascend950；冒烟芯片 Ascend960（dav_9201）。
- 本次为上游 Track-A 大扩充（84 → 118 → 172）后在 x86 上**全量重跑 172 个单元**：adv_api 计算子库、门控激活（SwiGLU/ReGLU）、归约家族（Mean/Sum/Any/All/Prod/XorSum 及 block/whole/pair 粒度）、量化（AscendQuant/Dequant/AntiQuant/Quantize）、softmax 变体（Flash V2/V3、反向）、格式转换（ConfusionTranspose/TransData）、交织/gather、判断等。**172 个单元全部一次通过、无需任何 x86 专属改动**（新单元本就在 ACL 释放前打印标记，且无写死架构的 include 路径）。

## 支持 x86 所需的 3 处修改

1. **安装脚本按架构自适应**（`install.sh`）：`uname -m` 决定 CANN `.run` 的下载地址/文件名与 miniforge 安装器（`linux-x86_64` vs `linux-aarch64`），可用 `ARCH=` 覆盖；cannsim wheel 与 toolkit 头文件目录改用通配查找，不再写死架构。
2. **PASS 标记在 ACL 释放之前打印**（host `main.cpp` 与模板）：在该 x86 包上 `aclFinalize()` 会结束进程 / 关闭仿真器的 stdout 捕获，导致写在 teardown **之后**的 `<TAG> SIMULATION PASSED` 从未被记录——尽管 `errors=0`，每个单元都被误判为 `sim_failed`。现在标记改在 teardown 之前打印（此处 `errors` 已是最终值）。对两种架构都有益，语义不变。
3. **包含路径按架构可移植**（`topk/CMakeLists.txt`、`gen.py`）：TopK 的 `topk_tiling.h` 会引入 `topk_utils_constants.h`，该头文件位于 toolkit 的 `<arch>-linux/asc/include/adv_api/sort`。原来写死成 `aarch64-linux`，现改为通配匹配实际存在的 `*-linux` 目录。

运行结果按架构打标签，互不覆盖（`run_one.sh` / `aggregate.py` 遵循 `REPORT_ARCH`，默认 `uname -m`；aarch64 沿用原始无后缀文件名）。

## 1. 仿真保真度：x86 复现了 aarch64 的 CAModel 指标

两个 CAModel 指标（指令执行数、执行耗时 ns）是器件级的，本应与宿主机无关。在全部 172 个单元上实测：

| 指标 | 平均 |Δ| | 中位 |Δ| | 最大 |Δ| |
|---|---|---|---|
| **指令数** | **0.038 %** | 0.000 % | 5.37 %（SyncAll） |
| **执行耗时 (ns)** | 2.34 % | 0.53 % | 16.8 %（小算子，固定开销主导） |

- **指令数基本完全一致**：172 个中 105 个逐位相等；其余仅相差个位数指令（基数 2 万~16 万，≤0.03 %）。纯计算/Cube/归一化/逻辑/判断/量化类核函数完全或近乎一致。
- **唯一 >0.1 % 的指令差异来自多核同步单元**：SyncAll（6256→5920，−5.37 %）与 IBSetWait（7780→7802，+0.28 %）。它们对 GM 标志位忙等轮询，轮询次数取决于核间调度——在**两种架构上都会逐次抖动**（aarch64 基线本身在上游多次重跑间也变化），属时序噪声，非正确性或跨架构问题。
- **执行耗时 (ns) 是 CAModel 的估算**，由指令轨迹推导，与指令数高度同步。较大的相对波动只出现在极小算子上——几纳秒的固定开销估算在不到 2500 ns 的总量里占比偏大。

**结论**：x86 toolkit 产出的器件级仿真与 aarch64 在计算类核函数上相差不到 0.05 %——对本项目而言两平台功能等价。

## 2. 本机效率（真正与宿主相关的指标）

CAModel 用宿主 CPU 跑 AI Core 仿真，因此真正由宿主决定的是**仿真器自身的墙钟耗时**（§1 的指标不是）。取自各单元 `record.log` 的 `Command executed successfully in Ns`（USER_APP 的 CAModel 执行墙钟）：

| | 数值 |
|---|---|
| 172 个单元总墙钟（串行） | **约 198 分钟**（11897 s） |
| 单元区间 | **40.5 s – 166.3 s** |
| 单元中位 / 平均 | 55 s / 69 s |
| 固定下限（最小约 200 指令单元） | **约 41 s** |

- 每个单元有 **约 41 s 的固定启动下限**（CAModel 初始化 + SoC 拉起 + 退出），与核函数大小无关——即便约 200 条指令的小算子也要约 45 s。中位（55 s）偏低，是因为新增算子多为贴近下限的小型逐元素/计算算子。
- 下限之上，墙钟**随指令数增长**：`Lgamma`（16 万指令）最慢 166 s，其次 `Digamma` 151 s；多核同步单元（`IBSetWait` 106 s、`SyncAll`）与指令密集的超越函数（`Power` 105 s、`Fmod` 102 s）次之。
- 本机处于 CAModel **最低**配置（16 核 / 31 GB，参考平台为 64 核 / 256 GB），故单算子墙钟偏慢——属预期，且与 §1 的（宿主无关）精度正交。

最慢 / 最快：

| 最慢 5 | 墙钟(s) | 指令 | | 最快 5 | 墙钟(s) | 指令 |
|---|--:|--:|---|---|--:|--:|
| Lgamma | 166 | 160040 | | TopK | 41 | 561 |
| Digamma | 151 | 77184 | | ScalarCast | 45 | 204 |
| IBSetWait | 106 | 7802 | | PairReduceSum | 45 | 301 |
| Power | 105 | 41664 | | Duplicate | 45 | 204 |
| Fmod | 102 | 36544 | | BitwiseNot | 46 | 274 |

> 完整逐单元对比表见英文版 [PERF.x86_64.md](PERF.x86_64.md) 第 3 节，或汇总表 [`INDEX.x86_64.md`](INDEX.x86_64.md)。
