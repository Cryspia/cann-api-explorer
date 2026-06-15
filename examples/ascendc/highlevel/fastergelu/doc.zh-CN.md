# Ascend C · FasterGelu

- 分类：矢量计算 / highlevel（高阶激活，`dst = OP(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件支持 float/half
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 激活接口 `FasterGelu`，对 LocalTensor 按元素计算 GELU 的快速近似。
按 toolkit 头，近似公式为：
```
FasterGelu(x) = x / (1 + e^(-1.702 * x))
```

## 函数原型（count 模式，摘自 toolkit 头 `gelu.h`）
```cpp
void FasterGelu(const LocalTensor<T>& dstLocal, const LocalTensor<T>& srcLocal, const uint32_t dataSize)
```
注：在 __NPU_ARCH__==3510 上，`FasterGelu`（count 模式）无需临时 buffer，仅支持 float/half。

## 最简 example 设计
- src 全填 `0.0` → 期望 `dst ≈ 0.0`（FasterGelu(0) = 0，稳定精确点），
  host 侧逐元素校验，采用放宽容差（tol 5e-2）以容忍近似误差。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
