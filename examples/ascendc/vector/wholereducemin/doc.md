# Ascend C / WholeReduceMin

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`WholeReduceMin` finds the min of each repeat. With `ORDER_ONLY_VALUE` only the value is emitted (1 element/repeat).

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void WholeReduceMin(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t mask, int32_t repeatTime, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride, ReduceOrder order);
```

## Minimal example design
- src[i] = i%64 -> each repeat is 0..63, min = 0.0 (8 outputs).
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
