# Ascend C · DeepNorm (high-level, 5-field hand-filled Tiling)

- Category: high-level adv_api / normalization
- Covered API: `AscendC::DeepNorm`
- include: `lib/normalization/deepnorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / DeepNorm

## Functionality
`DeepNorm<T,isReuseSrc,isBasicBlock>(dst, mean, rstd, x, gx, beta, gamma, tmp, alpha, epsilon, tiling)`:
`y = LayerNorm(alpha*x + gx) * gamma + beta`, with mean/var computed over the H dimension. `gx` is the Sublayer(x) residual term.

## Key point: 3510 only reads 5 shape fields, mean coefficient computed internally
`DeepNormTiling` has 20+ fields; the 3510 impl only references `bLength / sLength / hLength / originalHLength / oneTmpSize`:
- `GetDeepnormPara` uses `hLength` to compute `hRepeatTimes/hTailSize/hDim`; `para.hDim = hLength` is the divisor of the mean, **with no need to back out the coefficient** (compared with LayerNorm, which has to back out `k2Rec·k2RRec=1/R`, DeepNorm is much more straightforward).
- `alpha` realizes `alpha*x + gx` via `Reg::Axpy`.
- `originalHLength` is only used for the `isBasicBlock` check (H%64==0, etc.); `oneTmpSize` is only asserted `>0`.
```cpp
AscendC::tiling::DeepNormTiling t;
t.bLength = 1; t.sLength = 1; t.hLength = 8; t.originalHLength = 8; t.oneTmpSize = 256;
AscendC::DeepNorm<float, false, false>(zL, meanL, rstdL, xL, gxL, betaL, gammaL, tmp, /*alpha*/2.0f, /*eps*/1e-5f, t);
```

## Minimal example design
- `[B=1,S=1,H=8]`, `x=1, gx=0, alpha=2 -> eff=alpha*x+gx=2` all equal -> `var=0`.
- `gamma=1, beta=3` -> `dst = 0*gamma + beta = 3`, `mean=2`. Single core, host verify errors=0 (instr~854).
