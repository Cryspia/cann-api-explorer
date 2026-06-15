# Ascend C · SoftmaxFlashV3 (high-level, with Tiling)

- Category: high-level adv_api / activation (FlashAttention-2 online softmax, v3 variant)
- Covered API: `SoftmaxFlashV3`, `SoftMaxTilingFunc` (constructs tiling on the device side), `SoftMaxParams`
- include: `lib/activation/softmaxflashv3.h`
- Source: CANN 9.1.0 Ascend C API Reference / SoftmaxFlashV3

## Functionality
`SoftmaxFlashV3<T,U,isUpdate>(dst, mean, sum, max, src, expMax, inMean, inSum, inMax, sharedTmpBuffer, tiling, params)`:
the v3 online softmax used inside FlashAttention-2. Compared with FlashV2 it adds an explicit
per-row mean step and a shift term. For the first block (`isUpdate = false`):
```
rowMeanGlobal = rowsum(x) / K           (per-row)
mean          = rowMeanGlobal
x'            = x - meanTmp * alpha/(1-alpha)
max           = rowmax(x')
y             = exp(x' - max)           (un-normalized, NOT divided by sum)
sum           = rowsum(y)
```
The API fixes the dtypes: `T` (src / dst / expMax) = `half`, `U` (mean / sum / max) = `float`
(enforced by a `static_assert` in the impl).

## Key point: device-side tiling + FlashV3 params
The tiling is the same `SoftMaxTiling` as plain softmax, so it is built inside the kernel with
the device-callable `AscendC::SoftMaxTilingFunc`. The FlashV3-specific knobs live in
`AscendC::SoftMaxParams`:
```cpp
AscendC::SoftMaxParams params;
params.srcM = M; params.srcK = K; params.oriSrcM = M; params.oriSrcK = K;
params.splitMeanCnt = 1;   // K=64 -> kRepeatTime=1, so the single-split scheme is well-formed
AscendC::SoftmaxFlashV3<half, float, false>(z, mean, sum, max, x, expMax,
                                            inMean, inSum, inMax, tmp, tiling, params);
```
- `SoftMaxTiling` is in `AscendC::tiling`; `SoftMaxParams`/`SoftMaxTilingFunc`/`SoftmaxFlashV3` are in `AscendC`.
- The default `splitMeanCnt` is 8; with K=64 the vector repeat covers the whole row (kRepeatTime=1),
  so `splitMeanCnt` must be 1 to avoid an underflow of `remainRepeatTime = kRepeatTime - splitMeanCnt`.
- Reduce outputs (mean / sum / max) use the AscendC block layout: value in column 0 of each 8-float block.

## Minimal example design
- Shape `[M,K]=[8,64]`, `isUpdate = false` (first block), so inMean / inSum / inMax / expMax are unused.
- `x` = all `0.0` (half). Every per-row statistic collapses to 0, so the shifted input `x' = 0`.
- Therefore `mean = 0`, `max = 0`, `y = exp(0) = 1.0` (un-normalized), `sum = K = 64`.
- Expected: dst all `1.0`; per-row `mean = 0`, `max = 0`, `sum = 64`. Single-core execution,
  host verification with half encode/decode (tol 1e-2 on half values). See `kernel.cpp` / `main.cpp`,
  results in `RESULT.md`.
