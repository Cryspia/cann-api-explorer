# Ascend C · BlockReduceMin

- 分类：矢量计算 / vector（粒度归约）
- dtype：float（本例）；头文件另支持 half（部分另支持整型）
- 原文：CANN 9.1.0 Ascend C API 参考，`kernel_operator_vec_reduce_intf.h`

## 功能
`BlockReduceMin` 取每个 32B block（8 个 float）的最小值。每 repeat 64 float → 8 个最小值。

## 函数原型（count 模式，3510，摘自 `kernel_operator_vec_reduce_intf.h`）
```cpp
void BlockReduceMin(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## 最简 example 设计
- src[i]=i%64。第 b 个 block 最小值=b*8 → 每 repeat 输出 0,8,16,...。
- SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
