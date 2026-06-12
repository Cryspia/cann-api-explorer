# Ascend C / Mul

- Category: vector compute / vector (binary, `dst = OP(src0, src1)` element-wise)
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Mul`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_binary_intf.h`)
```cpp
void Mul(const LocalTensor<T>& dst, const LocalTensor<T>& src0, const LocalTensor<T>& src1, const int32_t& count)
```

## Minimal example design
- src0=`3.0`, src1=`2.0` -> expect `dst ~ 6.0`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
