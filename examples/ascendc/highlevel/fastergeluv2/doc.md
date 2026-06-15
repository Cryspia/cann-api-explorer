# Ascend C / FasterGeluV2

- Category: vector compute / highlevel (high-level activation, `dst = OP(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header supports float/half
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC activation interface `FasterGeluV2`, a second fast approximation of GELU computed
element-wise over a LocalTensor. Per the toolkit header:
```
sgn(x) = (x + 1e-12) / |x + 1e-12|
FasterGeluV2(x) = x * (sgn(x) * [(-0.1444) * (clip(|0.7071 * x|, max=1.769) - 1.769)^2 + 0.5] + 0.5)
```

## Function prototype (count mode, taken from toolkit header `gelu.h`)
```cpp
void FasterGeluV2(const LocalTensor<T>& dstLocal, const LocalTensor<T>& srcLocal, const uint32_t dataSize)
```
Note: on __NPU_ARCH__==3510 `FasterGeluV2` (count mode) requires no temporary buffer and
only supports float/half.

## Minimal example design
- src filled entirely with `0.0` -> expect `dst ~ 0.0`. Because the whole expression is multiplied
  by `x`, FasterGeluV2(0) = 0 regardless of the inner approximation, which makes it a stable
  exact verification point. Host-side element-wise verify with a loose tolerance (tol 5e-2).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
