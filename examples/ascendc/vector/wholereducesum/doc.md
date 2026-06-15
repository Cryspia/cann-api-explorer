# Ascend C / WholeReduceSum

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`WholeReduceSum` reduces all effective elements (mask) of each repeat to one sum.

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void WholeReduceSum(const LocalTensor<U>& dst, const LocalTensor<T>& src, int32_t mask, int32_t repeatTime, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## Minimal example design
- 8 repeats x 64 floats, all `1.0` -> each repeat sum = 64.0 (8 outputs).
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
