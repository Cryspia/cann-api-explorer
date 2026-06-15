# Ascend C · Normalize (high-level, statistics/normalization)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / normalization
- Covered API: `Normalize`
- include: `lib/normalization/normalize.h`
- Source: CANN 9.1.0 Ascend C API Reference / Normalize

## Function
```
Normalize<U, T, isReuseSource, config>(
    output,           // LocalTensor<T>,     shape [A, R]
    outputRstd,       // LocalTensor<float>, shape [A]
    inputMean,        // LocalTensor<float>, shape [A]
    inputVariance,    // LocalTensor<float>, shape [A]
    inputX,           // LocalTensor<T>,     shape [A, R]
    gamma,            // LocalTensor<U>,     shape [R]
    beta,             // LocalTensor<U>,     shape [R]
    sharedTmpBuffer,  // LocalTensor<uint8_t>
    epsilon,          // float
    para)             // NormalizePara { aLength, rLength, rLengthWithPadding }
```
Given a precomputed per-row mean and variance, Normalize is the "second half" of LayerNorm:
```
rstd   = rsqrt(variance + epsilon)
output = (x - mean) * rstd * gamma + beta
```
`outputRstd` is always `float`. There is also an overload without `sharedTmpBuffer` (it pops a stack buffer internally).

## Key point: no host tiling, only NormalizePara
On this arch (3510) the impl reads only `para.{aLength, rLength, rLengthWithPadding}`:
```cpp
AscendC::NormalizePara para;
para.aLength = A;                 // number of rows
para.rLength = R;                 // reduce length
para.rLengthWithPadding = RPAD;   // 32B-aligned row stride
AscendC::Normalize<float, float>(y, rstd, mean, var, x, gamma, beta, tmp, eps, para);
```
The default `NormalizeConfig` is the `AR` reduce pattern with gamma and beta applied.

## Minimal example design
- `[A=2, R=8]`, `RPAD=8`. mean `0`, variance `1`, eps `0` -> `rstd = rsqrt(1) = 1`.
- x all `1.0`, gamma all `1.0`, beta all `0.0` -> `output = (1-0)*1*1+0 = 1`, and `outputRstd = 1`.
- The `[A]`-shaped tensors (mean/variance/rstd) are stored padded to 8 floats (32B) so `DataCopy` is 32B-aligned; the padding entries keep `variance=1` to avoid an `rsqrt(0)` in the broadcast path.
- `outputRstd` is routed through its own `VECOUT` queue so the vector-pipe store is synchronized before the MTE3 copy-out (otherwise it races and reads back `0`).
- Single core, host verifies `errors=0`. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
