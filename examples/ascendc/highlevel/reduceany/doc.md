# Ascend C · ReduceAny
- highlevel / logical "any" reduction　- `AscendC::ReduceAny`　- include: `adv_api/reduce/reduce.h`
- `ReduceAny<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`: logical "any element true" along the last (R) axis. `srcShape = {dimA, dimR}`.
- Constraints: data type `uint8_t`/`float`. For `uint8_t` the result is the max element, so any nonzero -> nonzero, all-zero -> 0.
- Example: shape [1,64], all zeros except `x[7]=1` -> dst[0] = 1. errors=0.
