# Ascend C / Rsqrt

- Category: vector compute / vector (unary, `dst = OP(src)` element-wise)
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Rsqrt`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_unary_intf.h`)
```cpp
void Rsqrt(const LocalTensor<T>& dst, const LocalTensor<T>& src, const int32_t& count)
```

## Minimal example design
- src filled entirely with `4.0` -> expect `dst ~ 0.5`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
