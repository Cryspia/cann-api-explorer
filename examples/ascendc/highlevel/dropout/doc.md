# Ascend C · DropOut (high-level adv_api)

- Category: high-level / filter
- API covered: `AscendC::DropOut<T>(dst, src, mask, sharedTmpBuffer, keepProb, DropOutShapeInfo)`
- include: `lib/filter/dropout.h` (kernel)
- Status: **passed** on this 3510 (Ascend950PR) card-free simulation.

## Why DropOut is deterministically verifiable
DropOut in Ascend C is **not** internally random. The keep/drop decision is carried by a
caller-supplied `mask` tensor (`uint8`), so for a fixed mask the output is fully reproducible.
The internal RNG that produces a mask lives in a separate generation step, not in this API.

In byte-mask mode the 3510 implementation computes, per element:

```
dst[i] = (1 / keepProb) * (float)mask[i] * src[i]
```

`mask[i]` is cast from `uint8` to the compute dtype and multiplied in. `dropOutMode = 0`
(the default) auto-selects the layout: when `srcLastAxis == maskLastAxis` it takes the
byte-aligned path.

## Design
- Shape `[firstAxis = 1, lastAxis = 64]`, `dtype = float`.
- `src = 3.0`, `mask = 1` (keep everything), `keepProb = 0.5`.
- Expected `dst = (1 / 0.5) * 1 * 3.0 = 6.0` for every element. Verified on host.

## Run
```
bash harness/run_one.sh examples/ascendc/highlevel/dropout
```
