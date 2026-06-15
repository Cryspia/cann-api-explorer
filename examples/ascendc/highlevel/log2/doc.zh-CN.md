# Ascend C · Log2

- 分类：矢量计算 / highlevel（高阶数学，`dst = OP(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 数学接口 `Log2`，对 LocalTensor 按元素计算以 2 为底的对数。
这是独立函数，与 `Log`（自然对数）和 `Log10` 不同。

## 函数原型（count 模式，摘自 toolkit 头 `log.h`）
```cpp
void Log2(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, uint32_t calCount)
```
注：在 __NPU_ARCH__==3510 上，仅 count 的重载只有当 `T == half` 时才会内部弹出栈 tmp buffer；
`float` 无需临时 buffer。

## 最简 example 设计
- src 全填 `8.0` → 期望 `dst ≈ 3.0`（log2(8) = 3），host 侧逐元素校验（tol 5e-3）。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
