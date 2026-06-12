# Ascend C · MulCast
- 矢量 / 乘+类型转换　- `AscendC::MulCast`　- include：`kernel_operator.h`
- `MulCast<T,U>(dst, src0, src1, count)`：dst[i]=(T)(src0[i]*src1[i])。**dtype 组合受限**：T=int32,U=int64 / T=int8|uint8,U=half。
- 例：src=int64 [0..63]×2 → dst=int32 [0,2,4,..]。errors=0（instr≈344）。
