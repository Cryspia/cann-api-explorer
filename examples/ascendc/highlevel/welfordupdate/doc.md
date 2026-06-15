# Ascend C · WelfordUpdate (high-level, normalization)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / normalization
- Covered API: `WelfordUpdate` (the Welford online update step)
- include: `lib/normalization/layernorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / WelfordUpdate

## Functionality
`WelfordUpdate` performs one online step of Welford's algorithm: it folds a new sample `x`
into the running mean/variance accumulators. It complements the existing `welfordfinalize`
unit, which merges per-block accumulators at the end.

The 3510 float-path per-element formula (from `welford_3510_impl.h`) is:
```
tmp     = x - inMean
outMean = inMean + nRec * tmp            (nRec = 1/n, the running count reciprocal)
outVar  = inVar  + tmp * (x - outMean)   (running M2 accumulation)
```

## Measured signature
```cpp
template <typename T, typename U, bool isReuseSource = false,
          const WelfordUpdateConfig& config = WFUPDATE_DEFAULT_CFG>
void WelfordUpdate(const LocalTensor<U>& outputMean, const LocalTensor<U>& outputVariance,
                   const LocalTensor<U>& inputMean, const LocalTensor<U>& inputVariance,
                   const LocalTensor<T>& inputX, const WelfordUpdateParam& para);
// also a no-tmp overload (used here) and a sharedTmpBuffer overload.

struct WelfordUpdateParam { uint32_t rnLength; uint32_t abLength; uint32_t abComputeLength; float nRec; };
```
- `T` supports `half`/`bfloat16`/`float`; `U` is always `float`.
- `para.abComputeLength` (= K) is the number of elements processed; `para.nRec = 1/n`.

## Minimal example design
- Single tile, `ELEM = 8` floats (32B). `abLength = abComputeLength = 8`.
- `inMean = 1`, `inVar = 0`, `x = 3`, `nRec = 0.5` (n = 2):
  `outMean = 1 + 0.5*(3-1) = 2`, `outVar = 0 + (3-1)*(3-2) = 2`.
- Host verifies `outMean == 2` and `outVar == 2`. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
