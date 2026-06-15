# Ascend C · BitwiseAnd

- 分类：矢量计算 / highlevel（整型按位运算，adv_api 简单 count 模式）
- dtype：整型；src 与 dst 同为类型 `T`。本 SoC 的 Level-2 整型支持：`int16`/`uint16`/`int64`/`uint64`（**不支持 `int32`**），故本单元使用 `int16`。
- 关系：本算子是逐元素按位与的 **adv_api（`lib/math`）count 模式**入口；同一运算也由更底层的矢量内联 `AscendC::And` 提供（见 `../../vector/and/`）；语义相同，API 层级不同。
- 原文：见 CANN 9.1.0 Ascend C API 参考（`adv_api/math/bitwise_and.h`）

## 功能
`BitwiseAnd`：两个整型张量逐元素按位与，`dst[i] = src0[i] & src1[i]`。

## 函数原型（count 模式，摘自 toolkit 头 `bitwise_and.h`，__NPU_ARCH__==3510）
```cpp
template <const BitwiseAndConfig& config, typename T>
void BitwiseAnd(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
                const LocalTensor<T>& src1, const uint32_t count)
```

## 最简 example 设计
- 输入：src0 = `12`（0b1100），src1 = `10`（0b1010）→ 期望 `dst = 8`（0b1000）。整型精确匹配。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
