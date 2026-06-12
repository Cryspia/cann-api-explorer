# Ascend C / Cast

- Category: vector compute / vector (cast, `dst<int32_t> = OP(src<float>, roundMode)` element-wise)
- dtype: float->int32_t (this example); the header also supports float, int32_t
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Cast`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_vconv_intf.h`)
```cpp
void Cast(const LocalTensor<T>& dst, const LocalTensor<U>& src, const RoundMode& roundMode, const uint32_t count)
```

## Minimal example design
- src(float) filled entirely with `3.0`, RoundMode=`CAST_RINT` -> expect `dst == 3`, host-side element-wise verify (exact match).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
