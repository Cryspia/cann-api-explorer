# Ascend C / Sign

- Category: vector compute / highlevel (high-level activation, `dst = OP(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Sign`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `sign.h`)
```cpp
void Sign(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, const uint32_t calCount)
```

## Minimal example design
- src filled entirely with `3.0` -> expect `dst ~ 1.0`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
