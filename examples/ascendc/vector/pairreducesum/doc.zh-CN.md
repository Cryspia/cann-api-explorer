# Ascend C · PairReduceSum

- 分类：矢量计算 / vector（粒度归约）
- dtype：float（本例）；头文件另支持 half（部分另支持整型）
- 原文：CANN 9.1.0 Ascend C API 参考，`kernel_operator_vec_reduce_intf.h`

## 功能
`PairReduceSum` 对相邻成对元素求和，输出元素数减半。

## 函数原型（count 模式，3510，摘自 `kernel_operator_vec_reduce_intf.h`）
```cpp
void PairReduceSum(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## 最简 example 设计
- 1 repeat × 64 float，全 `1.0` → 每对和 = 2.0（32 个输出）。
- SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
