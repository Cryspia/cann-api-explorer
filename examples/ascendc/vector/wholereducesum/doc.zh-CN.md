# Ascend C · WholeReduceSum

- 分类：矢量计算 / vector（粒度归约）
- dtype：float（本例）；头文件另支持 half（部分另支持整型）
- 原文：CANN 9.1.0 Ascend C API 参考，`kernel_operator_vec_reduce_intf.h`

## 功能
`WholeReduceSum` 将每个 repeat 的全部有效元素（mask 个）归约为一个和。

## 函数原型（count 模式，3510，摘自 `kernel_operator_vec_reduce_intf.h`）
```cpp
void WholeReduceSum(const LocalTensor<U>& dst, const LocalTensor<T>& src, int32_t mask, int32_t repeatTime, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## 最简 example 设计
- 8 repeats × 64 float，全 `1.0` → 每 repeat 和 = 64.0（8 个输出）。
- SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
