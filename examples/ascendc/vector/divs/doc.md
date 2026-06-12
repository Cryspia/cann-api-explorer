# Ascend C / Divs

- Category: vector compute / vector (vector-scalar, `dst = OP(src, scalar)` element-wise)
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Divs`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_binary_scalar_intf.h`)
```cpp
void Divs(const U& dst, const S& src0, const V& src1, const int32_t& count)
```

## Minimal example design
- src filled entirely with `6.0`, scalar `2.0` -> expect `dst ~ 3.0`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
