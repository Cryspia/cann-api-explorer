# Ascend C · ReduceAll
- highlevel / logical "all" reduction　- `AscendC::ReduceAll`　- include: `adv_api/reduce/reduce.h`
- `ReduceAll<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`: logical "all elements true" along the last (R) axis. `srcShape = {dimA, dimR}`.
- Constraints: data type `uint8_t`/`float`. For `uint8_t` the result is the min element, so all nonzero -> nonzero, any zero -> 0.
- Example: shape [1,64], all ones -> dst[0] = 1. errors=0.
