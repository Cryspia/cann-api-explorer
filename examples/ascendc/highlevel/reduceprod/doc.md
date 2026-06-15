# Ascend C · ReduceProd
- highlevel / product reduction　- `AscendC::ReduceProd`　- include: `adv_api/reduce/reduce.h`
- `ReduceProd<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`: product of all elements along the last (R) axis. `srcShape = {dimA, dimR}`.
- Constraints: data type `float` only on this (3510) architecture.
- Example: shape [1,64], all 1.0 except `x[3]=2.0` -> product = 2.0 -> dst[0] = 2.0. errors=0.
