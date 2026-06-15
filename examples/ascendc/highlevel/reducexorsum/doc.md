# Ascend C · ReduceXorSum
- highlevel / bitwise-xor reduction　- `AscendC::ReduceXorSum`　- include: `adv_api/reduce/reduce_xor_sum.h`
- `ReduceXorSum<T>(dst, src0, src1, sharedTmpBuffer, calCount)`: f = `sum_i(src0[i] ^ src1[i])` (per-element bitwise XOR followed by a reduce-sum).
- Constraints: data type `int16_t` only on this (3510) architecture; `dst` minimum shape is 16; the scalar result lands in `dst[0]`.
- Example: length 64, `src0[i]=i` (0..63), `src1[i]=5`. The host recomputes `sum(i^5)` and compares against `dst[0]`. errors=0.
