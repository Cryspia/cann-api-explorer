# Ascend C / ClampMin

- Category: vector compute / highlevel (high-level math, `dst = max(src, scalar)` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC high-level math interface `ClampMin`, replaces every element less than `scalar` with `scalar` (i.e. `dst[i] = max(src[i], scalar)`).

## Function prototype (count mode, taken from toolkit header `clamp.h`)
```cpp
template <typename T, bool isReuseSource = false>
void ClampMin(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor,
              const T scalar, const uint32_t calCount);
```

## Minimal example design
- src filled entirely with `1.0`, `scalar = 3.0` -> expect `dst ~ 3.0`; host-side element-wise verify (tol 5e-3).
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
