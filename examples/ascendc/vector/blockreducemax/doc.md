# Ascend C / BlockReduceMax

- Category: vector compute / vector (granularity reduction)
- dtype: float (this example); header also supports half (and integer types for some)
- Source: CANN 9.1.0 Ascend C API reference, `kernel_operator_vec_reduce_intf.h`

## Functionality
`BlockReduceMax` takes the maximum of each 32B block (8 floats). 64 floats per repeat -> 8 maxima.

## Function prototype (count mode, 3510, from `kernel_operator_vec_reduce_intf.h`)
```cpp
void BlockReduceMax(const LocalTensor<T>& dst, const LocalTensor<T>& src, int32_t repeatTime, int32_t mask, int32_t dstRepStride, int32_t srcBlkStride, int32_t srcRepStride);
```

## Minimal example design
- src[i] = i%64 (each repeat is 0..63). Block b (8 floats) max = b*8+7 -> outputs 7,15,23,... per repeat.
- SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp`; run results in `RESULT.md`.
