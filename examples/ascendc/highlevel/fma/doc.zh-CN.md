# Ascend C · Fma

- 分类：矢量计算 / highlevel（高阶 math，融合乘加 `dst = src0 * src1 + src2` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 高阶 math 接口 `Fma`，对三个输入 LocalTensor 逐元素计算融合乘加：`dst[i] = src0[i] * src1[i] + src2[i]`。

## 函数原型（count 模式，摘自 toolkit 头 `fma.h`）
```cpp
template <const FmaConfig& config = DEFAULT_FMA_CONFIG, typename T>
void Fma(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
         const LocalTensor<T>& src1, const LocalTensor<T>& src2, const uint32_t count);
```

## 最简 example 设计
- src0 = `2.0`，src1 = `3.0`，src2 = `1.0` → 期望 `dst = 2*3 + 1 = 7.0`；host 侧逐元素校验（tol 5e-3）。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
