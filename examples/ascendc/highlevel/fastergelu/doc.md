# Ascend C / FasterGelu

- Category: vector compute / highlevel (high-level activation, `dst = OP(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header supports float/half
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC activation interface `FasterGelu`, a fast approximation of GELU computed element-wise
over a LocalTensor. Per the toolkit header the approximation is:
```
FasterGelu(x) = x / (1 + e^(-1.702 * x))
```

## Function prototype (count mode, taken from toolkit header `gelu.h`)
```cpp
void FasterGelu(const LocalTensor<T>& dstLocal, const LocalTensor<T>& srcLocal, const uint32_t dataSize)
```
Note: on __NPU_ARCH__==3510 `FasterGelu` (count mode) requires no temporary buffer and
only supports float/half.

## Minimal example design
- src filled entirely with `0.0` -> expect `dst ~ 0.0` (FasterGelu(0) = 0, a stable exact point),
  host-side element-wise verify with a loose tolerance (tol 5e-2) to tolerate the approximation.
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
