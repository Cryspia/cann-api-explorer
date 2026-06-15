# Ascend C · FasterGeluV2

- 分类：矢量计算 / highlevel（高阶激活，`dst = OP(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件支持 float/half
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 激活接口 `FasterGeluV2`，对 LocalTensor 按元素计算 GELU 的第二种快速近似。
按 toolkit 头：
```
sgn(x) = (x + 1e-12) / |x + 1e-12|
FasterGeluV2(x) = x * (sgn(x) * [(-0.1444) * (clip(|0.7071 * x|, max=1.769) - 1.769)^2 + 0.5] + 0.5)
```

## 函数原型（count 模式，摘自 toolkit 头 `gelu.h`）
```cpp
void FasterGeluV2(const LocalTensor<T>& dstLocal, const LocalTensor<T>& srcLocal, const uint32_t dataSize)
```
注：在 __NPU_ARCH__==3510 上，`FasterGeluV2`（count 模式）无需临时 buffer，仅支持 float/half。

## 最简 example 设计
- src 全填 `0.0` → 期望 `dst ≈ 0.0`。由于整个表达式都乘以 `x`，
  无论内部近似如何，FasterGeluV2(0) = 0，因此是稳定的精确校验点。
  host 侧逐元素校验，采用放宽容差（tol 5e-2）。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
