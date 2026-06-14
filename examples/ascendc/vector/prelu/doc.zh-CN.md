# Ascend C / Prelu

- 分类：矢量计算 / vector（二元，`dst = OP(src0, src1)` 逐元素）
- dtype：float（本例）；头文件也支持 half, float
- 来源：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 矢量计算接口 `Prelu`，对 LocalTensor 逐元素计算（PReLU：src0>0 取 src0，否则 src0*src1，src1 为斜率）。

## 函数原型（count 模式，取自 toolkit 头文件 `kernel_operator_vec_binary_intf.h`）
```cpp
void Prelu(const LocalTensor<T>& dst, const LocalTensor<T> &src0, const LocalTensor<T> &src1, const uint32_t count)
```

## 最简 example 设计
- src0=`-3.0`，src1=`0.5` → 期望 `dst ~ -1.5`，host 侧逐元素校验（tol 1e-3）。
- 总长 8*2048，8 核，double buffer；构建 SOC `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`；运行结果见 `RESULT.md`。
