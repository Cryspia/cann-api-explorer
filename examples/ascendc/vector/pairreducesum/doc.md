# Ascend C / PairReduceSum

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`PairReduceSum` sums each adjacent (parity) pair of elements, halving the element count.

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void PairReduceSum(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## Minimal example design
- 1 repeat x 64 floats, all `1.0` -> each pair sum = 2.0 (32 outputs).
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
