# Ascend C / ReduceSum

- Category: vector compute / vector (reduction, `dst[0] = OP(src[0..count], tmpBuffer)` (256 elements -> scalar))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `ReduceSum`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_reduce_intf.h`)
```cpp
void ReduceSum(const LocalTensor<T>& dst, const LocalTensor<T>& src, const LocalTensor<T>& sharedTmpBuffer, const int32_t count)
```

## Minimal example design
- src filled entirely with `1.0` -> expect `dst ~ 256.0`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
