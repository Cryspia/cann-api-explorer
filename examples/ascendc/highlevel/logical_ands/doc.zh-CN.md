# Ascend C · LogicalAnds

- 分类：矢量计算 / highlevel（逻辑 / 判断类，输出 bool，adv_api 简单 count 模式）
- dtype：dst 为 `bool`（头文件要求 bool 输出）；src0 为张量，src1 为**标量**。本例：src0 `float`，标量 `float` → dst `bool`。
- 关系：本算子是 `LogicalAnd`（tensor-tensor，见 `../logical_and/`）的 **tensor-scalar** 版本，逐元素逻辑与语义相同，只是 API 入口不同（第二个操作数为标量，广播到所有元素）。
- 原文：见 CANN 9.1.0 Ascend C API 参考（`adv_api/math/logical_ands.h`）

## 功能
AscendC adv_api 接口 `LogicalAnds`：张量与标量逐元素逻辑与，`dst = (src0 != 0) && (scalar != 0)`。结果以 `bool`（1 字节，0/1）写出。

## 函数原型（count 模式，摘自 toolkit 头 `logical_ands.h`，__NPU_ARCH__==3510）
```cpp
template <const LogicalAndsConfig& config, typename T, typename U, typename S>
void LogicalAnds(const LocalTensor<T>& dst, const U& src0, const S& src1, const uint32_t count)
// T = dst 类型（bool），U = src0 张量，S = src1 标量
```

## 最简 example 设计
- 输入：src0 = `1.0`（真），标量 = `1`（真）→ 期望 `dst = 1`（true）。host 侧把 `bool` 输出按 `uint8_t` 读取并校验 0/1。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
