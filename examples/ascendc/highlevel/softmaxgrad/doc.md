# Ascend C · SoftmaxGrad (high-level, with Tiling)

- Category: high-level adv_api / normalization (softmax backward)
- Covered API: `SoftmaxGrad`, `SoftMaxTilingFunc` (constructs tiling on the device side)
- include: `lib/activation/softmaxgrad.h`
- Source: CANN 9.1.0 Ascend C API Reference / SoftmaxGrad

## Functionality
`SoftmaxGrad(dst, grad, x, sharedTmpBuffer, tiling, isFront, shapeInfo)`: softmax backward over `[M,K]`
along the last dimension K, where `x` is the forward softmax output. With `isFront = false`:
```
sum = rowsum(grad * x)
y   = grad * x - sum * x
```
With `isFront = true` it instead returns `y = rowsum(grad * x)`.

## Key point: constructing Tiling inside the kernel (avoiding the host tiling framework)
This example reuses the device-callable `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
to build the `SoftMaxTiling` directly inside the kernel:
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftmaxGrad<float>(zLocal, gradLocal, xLocal, tmp /*uint8 sharedTmpBuffer*/, tiling, false, info);
```
- `SoftMaxTiling` is in namespace `AscendC::tiling`; `SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftmaxGrad` are in `AscendC`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>`; this example gives it 16KB of VECCALC space.
- Supported dtypes: half and float.

## Minimal example design
- Shape `[M,K]=[8,64]`.
- `x` = uniform forward softmax output `1/K = 0.015625` (each row sums to 1).
- `grad` = constant `c = 2.0` everywhere.
- Then `sum = rowsum(grad*x) = K*(c/K) = c = 2.0`, and `y = grad*x - sum*x = c/K - c/K = 0`.
- Expected output: all elements exactly `0.0`. Single-core execution, host element-wise verification (tol 1e-3).
  See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
