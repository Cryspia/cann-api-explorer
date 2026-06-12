# Ascend C · MulCast
- vector / multiply + type conversion　- `AscendC::MulCast`　- include: `kernel_operator.h`
- `MulCast<T,U>(dst, src0, src1, count)`: dst[i]=(T)(src0[i]*src1[i]). **dtype combinations are restricted**: T=int32,U=int64 / T=int8|uint8,U=half.
- Example: src=int64 [0..63]x2 -> dst=int32 [0,2,4,..]. errors=0 (instr≈344).
