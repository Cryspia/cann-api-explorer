# Ascend C / BlockReduceSum

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`BlockReduceSum` sums all elements within each 32B block (8 floats). One repeat covers 256B = 64 floats = 8 blocks, producing 8 block sums.

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void BlockReduceSum(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## Minimal example design
- 8 repeats x 64 floats, all `1.0` -> every block sum = 8.0 (8 outs/repeat x 8 = 64 outs).
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
