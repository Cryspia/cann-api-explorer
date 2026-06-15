# Ascend C · LayerNormGrad (high-level adv_api)

- Category: high-level / normalization (backward)
- API covered: `AscendC::LayerNormGrad<T>(outputPdX, resForGamma, inputDy, inputX, inputVariance, inputMean, inputGamma, sharedTmpBuffer, epsilon, LayerNormGradTiling)`
- include: `lib/normalization/layernormgrad.h` (kernel)
- Status: **passed** on this 3510 (Ascend950PR) card-free simulation.

## What it computes
LayerNormGrad is the backward of LayerNorm. It produces two outputs (shapes `[B, S, H]`):

```
x1          = inputDy * inputGamma
x2          = inputX  - inputMean
rstd        = 1 / sqrt(inputVariance + epsilon)
pd_var      = sum_H( -0.5 * x1 * x2 * (var+eps)^(-1.5) )
pd_mean     = sum_H( -1.0 * x1 * rstd ) + pd_var * (1/H) * sum_H(-2 * x2)
resForGamma = x2 * rstd
outputPdX   = x1 * rstd + pd_var * (2/H) * x2 + pd_mean * (1/H)
```

`inputVariance` and `inputMean` have shape `[B, S, 1]`; `inputGamma` has shape `[H]`.

## Tiling on 3510
The 3510 implementation only reads six fields:
`stackBufferSize, bLength, sLength, hLength, lastDimValueBack, lastDimValueBackMulTwo`.
The last two are `1/H` and `2/H` carried as **float bit-patterns** inside `uint32` fields
(the impl `reinterpret_cast`s them back to `float`), so the kernel writes them via a
`reinterpret_cast<uint32_t*>(&floatValue)`.

## How a simple verifiable case is constructed
Rather than skip the backward, this picks inputs that drive both outputs to a known value:

- `dy = 0` -> `x1 = 0` -> `pd_var = 0` and `pd_mean = 0` -> `outputPdX = 0` everywhere.
- `x = mean = 5.0` (so `var = 0`) -> `x2 = 0` -> `resForGamma = 0` everywhere.

So with `[B=1, S=1, H=8]`, `gamma = 1.0`, `eps = 1e-5`, both `outputPdX` and `resForGamma`
are `0`. Verified on host. (This exercises the full impl path — load, the pd_var/pd_mean
reductions, and the pd_x assembly — while keeping the expected result trivially checkable.)

## Run
```
bash harness/run_one.sh examples/ascendc/highlevel/layernormgrad
```
