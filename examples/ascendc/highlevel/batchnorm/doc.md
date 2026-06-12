# Ascend C · BatchNorm (high-level, 3-field hand-filled tiling)

- Category: high-level adv_api / normalization
- API covered: `AscendC::BatchNorm`
- include: `lib/normalization/batchnorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / BatchNorm

## Function
`BatchNorm<T,isReuseSrc,isBasicBlock>(output, outputMean, outputVariance, x, gamma, beta, tmp, epsilon, tiling)`:
`output = gamma * (x - mean) * rsqrt(var + eps) + beta`, where `mean = sum_b(x) / B`, **normalized along the first dim B**.
Memory layout `[B, F]` row-major, i.e. `srcLocal[b*F + f]`; mean/var/gamma/beta are all per-feature `[F]`.

## Key: 3510 reads only 3 fields, including the coefficient firstDimValueBack
`BatchNormTiling` has 20 fields; the 3510 impl only references `originalBLength / meanVarSize / firstDimValueBack`:
- `originalBLength = B` (batch length, the reduction dim); `meanVarSize = F` (feature length).
- `firstDimValueBack = 1/B`: the mean coefficient. Inside `ComputeOutputMean`, while accumulating over each b it does `Reg::Muls(srcReg, firstDimValueBack)`. **1/B works directly, no back-derivation needed.**
```cpp
AscendC::tiling::BatchNormTiling t;
t.originalBLength = B;            // 4
t.meanVarSize = F;               // 8
t.firstDimValueBack = 1.0f / B;  // 0.25
AscendC::BatchNorm<float, false, false>(zL, meanL, varL, xL, gammaL, betaL, tmp, /*eps*/1e-5f, t);
```
> Same idea as GroupNorm's `factor=1/(D*HW)`: the mean coefficient is the reciprocal of the element count along the normalization dim.

## Minimal example design
- `[B=4,F=8]`, `x` all `5` -> the 4 batch values per feature are equal -> `var=0`.
- `gamma=1, beta=2` -> `output = 0*gamma + beta = 2`, `mean=5`. Single core, host verify errors=0 (instr≈863).
