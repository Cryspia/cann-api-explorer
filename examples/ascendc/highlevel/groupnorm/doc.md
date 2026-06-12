# Ascend C · GroupNorm (high-level, with Tiling)

- Category: high-level adv_api / normalization
- Covered API: `GroupNorm`
- include: `lib/normalization/groupnorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / GroupNorm

## Function
`GroupNorm(output, outputMean, outputVariance, inputX, gamma, beta, sharedTmpBuffer, epsilon, tiling)`:
input `[N, C, H*W]`, `C = G*D`, normalized over `D*HW` elements per `(N, G)` group; gamma/beta are per-channel `[C]`.
`y = (x - mean)/sqrt(var + eps) * gamma + beta`.

## Key point: filling GroupNormTiling (3510 uses very few fields)
`AscendC::tiling::GroupNormTiling` has 30 fields, but the impl on this arch (3510) only reads `n/g/d/hw/factor` (the core compute uses them directly),
`dhwAlignSize` is only used to select the ShapeScope branch, and `oneTmpSize` is only asserted `>0`:
```cpp
AscendC::tiling::GroupNormTiling t;
t.n=N; t.c=C; t.g=G; t.d=D; t.hw=HW;
t.dhwAlignSize = D*HW;          // <= oneRepSize(=GetVecLen()/4) -> ShapeScope ONE
t.factor = 1.0f/(D*HW);         // mean coefficient 1/(D*HW)
t.oneTmpSize = 256;             // only needs > 0
AscendC::GroupNorm<float>(z, mean, var, x, gamma, beta, tmp, 1e-5f, t);
```
Compared with LayerNorm (regbase, which requires back-solving k2Rec·k2RRec=1/R), GroupNorm's `factor` is directly the mean coefficient, which is more straightforward.

## Minimal example design
- `[N=1, C=4, HW=16]`, `G=2, D=2` (D*HW=32 elements per group), x all `5.0`, gamma all `1.0`, beta all `0.0`, eps `1e-5`.
- Identical values within a group -> mean=5, var=0 -> output=(x-mean)·rstd·gamma+beta = `0`. Single core, host verifies errors=0.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
