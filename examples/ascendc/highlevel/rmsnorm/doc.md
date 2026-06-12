# Ascend C · RmsNorm (high-level, with Tiling)

- Category: high-level adv_api / normalization
- Covered API: `RmsNorm`
- include: `lib/normalization/rmsnorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / RmsNorm

## Functionality
`RmsNorm(dst, src, gamma, sharedTmpBuffer, epsilon, tiling)`:
`y = x / sqrt(mean(x^2) + eps) * gamma`, normalized along the last dimension H.

## Key point: hand-filling Tiling inside the kernel
RmsNorm **does not have** a device-side tiling func (unlike SoftMax). However, `AscendC::tiling::RmsNormTiling` has only 12 fields,
and with a fixed single-tile shape `[B,S,H]=[1,8,64]` (one loop, no tail) it can be safely hand-filled inside the kernel:
```cpp
AscendC::tiling::RmsNormTiling t;
t.bLength=B; t.sLength=S; t.hLength=H; t.originalHLength=H;
t.reciprocalOfHLength=1.0f/H;
t.mainBshLength=B*S*H; t.mainBsLength=B*S; t.mainBsLengthAlign=B*S;
t.loopRound=1; t.inputTailPos=0; t.tailBshLength=0; t.tailBsLength=0;
AscendC::RmsNorm<float>(zL, xL, gammaL, tmp, 1e-5f, t);
```
(In production, for multi-tile / large shapes these fields should be computed by the host-side tiling.)

## Minimal example design
- `[1,8,64]`, x all `2.0`, gamma all `1.0`, eps `1e-5`.
- rms = sqrt(mean(4)+eps) ~= 2.0 -> y = 2/2*1 ~= `1.0`. Single core, verified on host (tol 5e-3).
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
