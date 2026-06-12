# Ascend C · Divs

- 分类：矢量计算 / vector（矢量-标量，`dst = OP(src, scalar)` 逐元素）
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 矢量计算接口 `Divs`，对 LocalTensor 按元素计算。

## 函数原型（count 模式，摘自 toolkit 头 `kernel_operator_vec_binary_scalar_intf.h`）
```cpp
void Divs(const U& dst, const S& src0, const V& src1, const int32_t& count)
```

## 最简 example 设计
- src 全填 `6.0`，标量 `2.0` → 期望 `dst ≈ 3.0`，host 侧逐元素校验（tol 1e-3）。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
