# Ascend C / Fma

- Category: vector compute / highlevel (high-level math, fused multiply-add `dst = src0 * src1 + src2` element-wise (adv_api, simple count mode))
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC high-level math interface `Fma`, computes the element-wise fused multiply-add of three input LocalTensors: `dst[i] = src0[i] * src1[i] + src2[i]`.

## Function prototype (count mode, taken from toolkit header `fma.h`)
```cpp
template <const FmaConfig& config = DEFAULT_FMA_CONFIG, typename T>
void Fma(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
         const LocalTensor<T>& src1, const LocalTensor<T>& src2, const uint32_t count);
```

## Minimal example design
- src0 = `2.0`, src1 = `3.0`, src2 = `1.0` -> expect `dst = 2*3 + 1 = 7.0`; host-side element-wise verify (tol 5e-3).
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
