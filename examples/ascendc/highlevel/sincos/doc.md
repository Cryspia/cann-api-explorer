# Ascend C / SinCos

- Category: vector compute / highlevel (high-level math, dual output `dstSin = sin(src)`, `dstCos = cos(src)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC high-level math interface `SinCos`, computes sine and cosine of the input LocalTensor in a single call, writing them to two separate output LocalTensors.

## Function prototype (count mode, taken from toolkit header `sincos.h`)
```cpp
template <const SinCosConfig& config = DEFAULT_SINCOS_CONFIG, typename T>
void SinCos(const LocalTensor<T>& dst0, const LocalTensor<T>& dst1,
            const LocalTensor<T>& src, const uint32_t count);
```
- `dst0` receives `sin(src)`, `dst1` receives `cos(src)`.

## Minimal example design
- src filled entirely with `0.0` -> expect `dstSin ~ 0.0` and `dstCos ~ 1.0`; both outputs verified element-wise on host (tol 5e-3).
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
