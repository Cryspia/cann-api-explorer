# Ascend C · SoftMax (high-level, with Tiling)

- Category: high-level adv_api / normalization
- Covered API: `SoftMax`, `SoftMaxTilingFunc` (constructs tiling on the device side)
- include: `lib/activation/softmax.h`
- Source: CANN 9.1.0 Ascend C API Reference / SoftMax

## Functionality
`SoftMax(dst, src, sharedTmpBuffer, tiling, shapeInfo)`: performs softmax on `[M,K]` along the last dimension K,
`y = exp(x-max)/sum(exp(x-max))`.

## Key point: constructing Tiling inside the kernel (avoiding the host tiling framework)
High-level operators with `*Tiling` usually require the host side to compute the tiling and pass it in. This example instead uses the **device-callable**
`AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)` to construct it directly inside the kernel:
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftMax<float>(zLocal, xLocal, tmp /*uint8 sharedTmpBuffer*/, tiling, info);
```
- `SoftMaxTiling` is in the namespace `AscendC::tiling`; `SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftMax` are in `AscendC`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>`; this example gives it 16KB of VECCALC space.

## Minimal example design
- Shape `[M,K]=[8,64]`, src all filled with `1.0`. Softmax along K -> each element `1/64 = 0.015625`.
- Single-core execution, host element-wise verification (tol 1e-3). See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
