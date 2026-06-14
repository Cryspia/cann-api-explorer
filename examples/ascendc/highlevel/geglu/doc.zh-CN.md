# Ascend C · GeGLU

- 分类：矢量计算 / highlevel（高阶激活，门控 GELU `dst = src0 * GeLU(src1)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 高阶激活接口 `GeGLU`（门控 GELU）。据 toolkit 头注释 `GeGLU(x1, x2) = x1 * GeLU(x2)`，其中 `x1` 为 `src0`、`x2` 为 `src1`，即 `dst[i] = src0[i] * GeLU(src1[i])`。

## 函数原型（count 模式，摘自 toolkit 头 `geglu.h`）
```cpp
template <typename T, bool isReuseSource = false>
void GeGLU(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor0,
           const LocalTensor<T>& srcTensor1, uint32_t calCount);
```

## 最简 example 设计
- src0 = `1.0`，src1 = `0.0`。因 `GeLU(0) = 0`，结果恒为 `dst = 1 * 0 = 0.0`，与实现所用的 GELU 近似无关 → 稳健且可精确校验。host 侧逐元素校验（tol 5e-3）。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
