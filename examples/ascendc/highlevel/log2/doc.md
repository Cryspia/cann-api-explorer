# Ascend C / Log2

- Category: vector compute / highlevel (high-level math, `dst = OP(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC math interface `Log2`, computes the base-2 logarithm element-wise over a LocalTensor.
This is an independent function, distinct from `Log` (natural logarithm) and `Log10`.

## Function prototype (count mode, taken from toolkit header `log.h`)
```cpp
void Log2(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, uint32_t calCount)
```
Note: on __NPU_ARCH__==3510 the count-only overload internally pops a stack tmp buffer
only when `T == half`; for `float` no temporary buffer is required.

## Minimal example design
- src filled entirely with `8.0` -> expect `dst ~ 3.0` (log2(8) = 3), host-side element-wise verify (tol 5e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
