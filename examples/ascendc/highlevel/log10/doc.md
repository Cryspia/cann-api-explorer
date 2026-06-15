# Ascend C / Log10

- Category: vector compute / highlevel (high-level math, `dst = OP(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC math interface `Log10`, computes the base-10 logarithm element-wise over a LocalTensor.
This is an independent function, distinct from `Log` (natural logarithm) and `Log2`.

## Function prototype (count mode, taken from toolkit header `log.h`)
```cpp
void Log10(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, uint32_t calCount)
```
Note: on __NPU_ARCH__==3510 `Log10` (count mode) does not require a temporary buffer.

## Minimal example design
- src filled entirely with `1000.0` -> expect `dst ~ 3.0` (log10(1000) = 3), host-side element-wise verify (tol 5e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
