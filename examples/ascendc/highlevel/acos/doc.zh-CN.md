# Ascend C · Acos

- 分类：矢量计算 / highlevel（高阶激活，`dst = OP(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 矢量计算接口 `Acos`，对 LocalTensor 按元素计算。

## 函数原型（count 模式，摘自 toolkit 头 `acos.h`）
```cpp
void Acos(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, const uint32_t calCount)
```

## 最简 example 设计
- src 全填 `1.0` → 期望 `dst ≈ 0.0`，host 侧逐元素校验（tol 5e-3）。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
