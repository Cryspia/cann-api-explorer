# Ascend C · LayerNorm (high-level, with Tiling, this arch=3510 regbase)

- Category: high-level adv_api / normalization
- Covered API: `LayerNorm` (regbase `LayerNorm(Para, LayerNormSeparateTiling)` overload)
- include: `lib/normalization/layernorm.h`
- Source: CANN 9.1.0 Ascend C API Reference / LayerNorm

## Function
`y = (x - mean) / sqrt(var + eps) * gamma + beta`, normalized along the last dimension R.
Output `output[A,R]` + `outputMean[A]` + `outputRstd[A]`.

## Key point: this arch (3510) takes the regbase path, must use Para + SeparateTiling
The standard `LayerNorm(..., LayerNormTiling)` overload is only provided on arch 2002/2201. **3510 only exposes**
`LayerNorm<U,T>(out, mean, rstd, x, gamma, beta, eps, tmp, LayerNormPara, LayerNormSeparateTiling)`.
After taking the simple branch with `rLength <= sregLower` (=GetVecLen()/4, 64 on this machine), only a few fields need hand-filling:
```cpp
AscendC::LayerNormPara para;            // {aLength=A, rLength=R, rLengthWithPadding=R}
AscendC::tiling::LayerNormSeparateTiling t;  // the remaining fields can be 0
t.aLength=A; t.rLength=R; t.k2Rec=1.0f; t.k2RRec=1.0f/R;
AscendC::LayerNorm<float,float>(z, mean, rstd, x, gamma, beta, 1e-5f, tmp, para, t);
```

## Pitfall record (key)
The device implementation of mean is `mean = k2RRec · ReduceSum(k2Rec · x)`, i.e. **k2Rec × k2RRec must = 1/R**.
The first version set both to `1/R` -> product `1/R²` -> mean=192/4096=0.047 -> output=(3-0.047)·rstd ~= **7.9997** (simulation does not crash but the numbers are all wrong).
After changing to `k2Rec=1.0, k2RRec=1/R` (product 1/R), mean=3, var=0, output=0 ✓.
(In production the regbase coefficients k2Rec/k2RRec/rHeadLength/oneTmpSize etc. are computed by host tiling; the shipped header does not contain the formula, so you must read the device impl to back-solve them.)

## Minimal example design
- `[A,R]=[8,64]`, x all `3.0`, gamma all `1.0`, beta all `0.0`, eps `1e-5`.
- Each row mean=3, var=0 -> output=(x-mean)·rstd·gamma+beta = `0`. Single core, host verifies errors=0.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`. Instruction count 909.
