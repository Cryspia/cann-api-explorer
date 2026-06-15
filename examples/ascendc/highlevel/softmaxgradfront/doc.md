# Ascend C · SoftmaxGradFront (high-level, with Tiling)

- Category: high-level adv_api / activation (softmax backward, front part)
- Covered API: `SoftmaxGradFront`, `SoftMaxTilingFunc` (constructs tiling on the device side)
- include: `lib/activation/softmaxgrad.h`
- Source: CANN 9.1.0 Ascend C API Reference / SoftmaxGradFront

## Functionality
`SoftmaxGradFront(dst, grad, x, sharedTmpBuffer, tiling, shapeInfo)`: the "front" part of the
softmax backward over `[M,K]` along the last dimension K, where `x` is the forward softmax
output. It produces only the per-row reduction:
```
y = rowsum(grad * x)
```
(unlike `SoftmaxGrad` with `isFront=false`, which returns the full `grad*x - sum*x`). The 3510
impl writes one scalar per row into column 0 of that row's 32B block, so the output is
reduce-shaped (`M` rows, value in col 0 of each 8-float block).

## Key point: constructing Tiling inside the kernel (avoiding the host tiling framework)
This example reuses the device-callable `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
to build the `SoftMaxTiling` directly inside the kernel:
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftmaxGradFront<float>(zLocal, gradLocal, xLocal, tmp, tiling, info);
```
- `SoftMaxTiling` is in `AscendC::tiling`; `SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftmaxGradFront` are in `AscendC`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>`; this example gives it 16KB of VECCALC space.
- Supported dtypes: half and float.

## Minimal example design
- Shape `[M,K]=[8,64]`.
- `x` = uniform forward softmax output `1/K = 0.015625` (each row sums to 1).
- `grad` = constant `c = 2.0` everywhere.
- Then `y = rowsum(grad*x) = K*(c/K) = c = 2.0` for every row.
- Expected output: each row's scalar `= 2.0` (read at `y[row*8]`). Single-core execution,
  host per-row verification (tol 1e-3). See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
