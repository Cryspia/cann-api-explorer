# Ascend C / RepeatReduceSum

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`RepeatReduceSum` (3510/5102/3003/3113 only) sums the `mask` effective elements of each repeat into one value. It differs from `WholeReduceSum` by exposing an explicit `dstBlkStride` parameter and a different argument order (repeat first, then mask).

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void RepeatReduceSum(const LocalTensor<U>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstBlkStride, int32_t srcBlkStride, int32_t dstRepStride, int32_t srcRepStride);
```

## Minimal example design
- 8 repeats x 64 floats, all `1.0` -> each repeat sum = 64.0 (8 outputs).
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
