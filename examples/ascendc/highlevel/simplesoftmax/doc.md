# Ascend C · SimpleSoftMax (high-level, with Tiling)

- Category: high-level adv_api / activation (softmax)
- Covered API: `SimpleSoftMax`, `SoftMaxTilingFunc` (constructs tiling on the device side)
- include: `lib/activation/simplesoftmax.h`
- Source: CANN 9.1.0 Ascend C API Reference / SimpleSoftMax

## Functionality
`SimpleSoftMax(dst, inSum, inMax, src, sharedTmpBuffer, tiling, shapeInfo)`: the simplified
softmax over `[M,K]` along the last dimension K. It takes the per-row max and sum already
computed elsewhere and only performs the final normalization step:
```
y = exp(x - inMax) / inSum
```
This was confirmed against the 3510 implementation (`Sub`, then `Exp`, then `Div` by inSum).

## Key point: constructing Tiling inside the kernel (avoiding the host tiling framework)
This example reuses the device-callable `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
to build the `SoftMaxTiling` directly inside the kernel:
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SimpleSoftMax<float>(zLocal, sumLocal, maxLocal, xLocal, tmp, tiling, info);
```
- `SoftMaxTiling` is in namespace `AscendC::tiling`; `SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SimpleSoftMax` are in `AscendC`.
- `inMax` / `inSum` use the AscendC reduce block layout: the per-row scalar lives in column 0 of each 32B (8-float) block.
- Supported dtypes: half and float (plus the half-src / float-stats combination).

## Minimal example design
- Shape `[M,K]=[8,64]`.
- `x` = all `0.0`; `inMax` = `0.0` per row; `inSum` = `64.0` per row.
- Then `y = exp(0 - 0) / 64 = 1/64 = 0.015625` everywhere.
- Expected output: all elements `0.015625`. Single-core execution, host element-wise verification (tol 1e-4).
  See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
