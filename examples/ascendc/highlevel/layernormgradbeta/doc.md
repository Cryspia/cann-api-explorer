# Ascend C · LayerNormGradBeta (high-level adv_api)

- Category: high-level / normalization (backward)
- API covered: `AscendC::LayerNormGradBeta<T>(outputPdGamma, outputPdBeta, resForGamma, inputDy, sharedTmpBuffer, LayerNormGradBetaTiling)`
- include: `lib/normalization/layernormgradbeta.h` (kernel)
- Status: **passed** on this 3510 (Ascend950PR) card-free simulation.

## What it computes
This is the second stage of the LayerNorm backward: it reduces over the `(B, S)` axis to
produce the gamma/beta gradients (output shape `[H]`):

```
pd_gamma[h] = sum_bs( inputDy[bs, h] * resForGamma[bs, h] )
pd_beta[h]  = sum_bs( inputDy[bs, h] )
```

`resForGamma` is the `[B, S, H]` tensor produced by `LayerNormGrad`; `inputDy` is the same
`[B, S, H]` upstream gradient.

## Tiling on 3510
The 3510 implementation only reads `tiling.bsLength` and `tiling.hLength`, and requires
`hLength * sizeof(T)` to be a multiple of 32 bytes (here `8 * 4 = 32`, satisfied).

## Design
A non-trivial but exactly checkable case (no need to zero the gradients):

- Shape `[BS = 4, H = 8]`, `dtype = float`.
- `inputDy = 2.0`, `resForGamma = 3.0` (all constant).
- `pd_gamma[h] = 4 * (2 * 3) = 24.0`, `pd_beta[h] = 4 * 2 = 8.0`. Verified on host.

## Run
```
bash harness/run_one.sh examples/ascendc/highlevel/layernormgradbeta
```
