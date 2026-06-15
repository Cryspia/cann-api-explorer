# Ascend C · RepeatReduceSum

- 分类：矢量计算 / vector（粒度归约）
- dtype：float（本例）；头文件另支持 half（部分另支持整型）
- 原文：CANN 9.1.0 Ascend C API 参考，`kernel_operator_vec_reduce_intf.h`

## 功能
`RepeatReduceSum`（仅 3510/5102/3003/3113）将每个 repeat 的 `mask` 个有效元素归约为一个值。与 `WholeReduceSum` 的区别：多了显式的 `dstBlkStride` 参数，且参数顺序不同（先 repeat 后 mask）。

## 函数原型（count 模式，3510，摘自 `kernel_operator_vec_reduce_intf.h`）
```cpp
void RepeatReduceSum(const LocalTensor<U>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstBlkStride, int32_t srcBlkStride, int32_t dstRepStride, int32_t srcRepStride);
```

## 最简 example 设计
- 8 repeats × 64 float，全 `1.0` → 每 repeat 和 = 64.0（8 个输出）。
- SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
