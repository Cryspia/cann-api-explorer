# Ascend C / GeGLU

- Category: vector compute / highlevel (high-level activation, gated GELU `dst = src0 * GeLU(src1)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC high-level activation interface `GeGLU` (gated GELU). Per the toolkit header comment `GeGLU(x1, x2) = x1 * GeLU(x2)`, where `x1` is `src0` and `x2` is `src1`, so `dst[i] = src0[i] * GeLU(src1[i])`.

## Function prototype (count mode, taken from toolkit header `geglu.h`)
```cpp
template <typename T, bool isReuseSource = false>
void GeGLU(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor0,
           const LocalTensor<T>& srcTensor1, uint32_t calCount);
```

## Minimal example design
- src0 = `1.0`, src1 = `0.0`. Since `GeLU(0) = 0`, the result is exactly `dst = 1 * 0 = 0.0`, independent of the GELU approximation used by the implementation -> robust, exactly verifiable. Host-side element-wise verify (tol 5e-3).
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
