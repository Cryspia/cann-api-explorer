# Ascend C · ReduceXorSum
- highlevel / 按位异或归约　- `AscendC::ReduceXorSum`　- include：`adv_api/reduce/reduce_xor_sum.h`
- `ReduceXorSum<T>(dst, src0, src1, sharedTmpBuffer, calCount)`：f = `sum_i(src0[i] ^ src1[i])`（先逐元素按位异或，再做归约求和）。
- 约束：本架构（3510）仅支持 `int16_t`；`dst` 最小形状为 16；标量结果落在 `dst[0]`。
- 例：长度 64，`src0[i]=i`（0..63），`src1[i]=5`。host 端重算 `sum(i^5)` 与 `dst[0]` 比对。errors=0。
