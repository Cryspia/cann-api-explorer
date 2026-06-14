# Ascend C · LogicalAnd

- 分类：矢量计算 / highlevel（逻辑 / 判断类，输出 bool，adv_api 简单 count 模式）
- dtype：dst 为 `bool`（头文件要求 bool 输出）；src 支持 `bool/uint8_t/int8_t/half/bfloat16_t/uint16_t/int16_t/float/...`。本例：src `float` → dst `bool`。
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC adv_api 接口 `LogicalAnd`：逐元素逻辑与，`dst = (src0 != 0) && (src1 != 0)`，对 LocalTensor 按元素计算。结果以 `bool`（1 字节，0/1）写出。

## 函数原型（count 模式，摘自 toolkit 头 `logical_and.h`）
```cpp
void LogicalAnd(const LocalTensor<T>& dst, const LocalTensor<U>& src0, const LocalTensor<U>& src1, const uint32_t count)
```

## 最简 example 设计
- 输入：src0 = `1.0`，src1 = `0.0` → 期望 `dst = 0`（false）。host 侧把 `bool` 输出按 `uint8_t` 读取并校验 0/1。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
