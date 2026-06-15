> English: [docs/notcovered.md](../docs/notcovered.md)

# C 线 · 不覆盖的库（为何不覆盖 + 将来覆盖需要的条件）

> **前提**：本项目用 **cannsim 无卡仿真（CAModel）** 跑 Ascend C 核函数，CAModel 只对**编译进核函数、跑在 AI Core 上的 device 指令**做指令级仿真并出报告。下列库要么是 **host 侧上层框架**（不产 AI Core 指令）、要么**需真实驱动 / 多卡 / 专用硬件单元**，故不在无卡仿真覆盖范围内。本机仅装了 toolkit、无 NPU 驱动，因此这些只做文档说明，留给将来有真实环境时开展。

| 库 | 是什么 | 为何不覆盖 | 将来覆盖需要的条件 |
|----|--------|-----------|-------------------|
| **GE**（Graph Engine 图引擎） | 把整张计算图编译、优化、调度执行 | 是 host 侧的图编译/调度框架，不产 AI Core 指令仿真；依赖完整 runtime + 图编译器 + driver | 真实 NPU + driver + GE 运行时；验证方式是端到端图执行，非指令仿真 |
| **aolapi / AOL / aclnn**（算子加速库） | 经 `aclnn*` host API 调用的预编译高层算子库（类比 cuDNN）；350+ 个算子如 `aclnnConvolution` / `aclnnFlashAttention` | 是 host 侧加载/调用预编译算子二进制，不暴露可仿真的核函数源码——与 A 线的 Ascend C **核函数** API 是**不同层**；`aclnnop` 头随单独的算子库（nnal/opp）包发布，不在 toolkit 内 | 真实设备 + 算子二进制包；调用即黑盒，无指令级 trace |
| **HCCL**（集合通信库） | 多卡 AllReduce/AllGather/Broadcast/ReduceScatter 等 | 需**多卡** + 卡间网络（HCCS/RoCE），单机无卡仿真无对端 | ≥2 张真实 NPU + 互联网络；验证集合通信正确性与带宽 |
| **HIXL**（异构互联交换层） | 跨设备/异构单元的互联与数据交换 | 需多设备互联硬件，无卡仿真无互联拓扑 | 多设备互联硬件环境 |
| **ATB**（Ascend Transformer Boost） | Transformer 高层加速库（attention/FFN/PagedAttention 等封装） | host 侧高层封装，内部调 runtime + 算子库；不暴露单个可仿真核函数 | 真实设备 + runtime + 算子库；端到端模型推理验证 |
| **SiP**（System-in-Package 相关特性） | 封装级系统/专用硬件特性接口 | 依赖特定封装硬件特性，无卡仿真不提供 | 对应封装硬件 |
| **DVPP**（数字视觉预处理） | 图像/视频编解码、缩放、色彩转换等硬件单元 | 需**专用 DVPP 硬件单元**（非 AI Core），CAModel 不仿真该单元 | 真实设备的 DVPP 单元；验证编解码/预处理输出 |

## 与已覆盖三线的关系

- **A 线（已完成，172/172）**：Ascend C 核函数 API —— CAModel 指令级仿真的唯一对象，本项目核心。
- **B 线（已完成）**：Runtime/AscendCL host API —— 发射核函数的脚手架，见 [`runtime/host_api.md`](runtime/host_api.md)，不产指令报告但其正确性隐含在 A 线 172 个 PASSED 单元中。
- **C 线（本文档）**：上层框架 / 需硬件的库 —— 明确不在无卡仿真范围，记录边界与将来条件。

> 一句话边界：**能编译成 AI Core 指令、被 CAModel 仿真出报告的 → A 线已穷尽；host 脚手架 → B 线已记录；其余上层框架/需硬件的 → C 线本文档说明不覆盖原因。**
