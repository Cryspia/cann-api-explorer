# Ascend C · BitwiseNot

- 分类：矢量计算 / highlevel（整型按位运算，adv_api 简单 count 模式）
- dtype：整型；src 与 dst 同为类型 `T`。本 SoC 的 Level-2 整型支持：`int16`/`uint16`/`int64`/`uint64`（**不支持 `int32`**），故本单元使用 `int16`。
- 关系：本算子是逐元素按位取反（一补码）的 **adv_api（`lib/math`）count 模式**入口；同一运算也由更底层的矢量内联 `AscendC::Not` 提供（见 `../../vector/not/`）；语义相同，API 层级不同。
- 原文：见 CANN 9.1.0 Ascend C API 参考（`adv_api/math/bitwise_not.h`）

## 功能
`BitwiseNot`：整型张量逐元素按位取反，`dst[i] = ~src[i]`。对有符号补码整数 `~x = -(x+1)`，故 `~5 = -6`、`~0 = -1`。

## 函数原型（count 模式，摘自 toolkit 头 `bitwise_not.h`，__NPU_ARCH__==3510）
```cpp
template <const BitwiseNotConfig& config, typename T>
void BitwiseNot(const LocalTensor<T>& dst, const LocalTensor<T>& src, const uint32_t count)
```

## 最简 example 设计
- 输入：src = `5` → 期望 `dst = -6`（有符号 int16 补码）。整型精确匹配。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
