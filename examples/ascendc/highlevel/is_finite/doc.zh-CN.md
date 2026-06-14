# Ascend C · IsFinite

- 分类：矢量计算 / highlevel（逻辑 / 判断类，输出 bool，adv_api 简单 count 模式）
- dtype：支持的 (dst, src) 组合：(bool/half, half)、(bool/float, float)、(bool/bfloat16, bfloat16)。本例：src `float` → dst `bool`。
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC adv_api 接口 `IsFinite`：逐元素有限判断，`dst = isfinite(src)`，对 LocalTensor 按元素计算。结果以 `bool`（1 字节，0/1）写出。

## 函数原型（count 模式，摘自 toolkit 头 `is_finite.h`）
```cpp
void IsFinite(const LocalTensor<U>& dst, const LocalTensor<T>& src, uint32_t calCount)
```

## 最简 example 设计
- 输入：src = `1.0` → 期望 `dst = 1`（true）。host 侧把 `bool` 输出按 `uint8_t` 读取并校验 0/1。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
