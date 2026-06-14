# Ascend C · FusedMulAdd
- vector / fused multiply-add　- `AscendC::FusedMulAdd`　- include: `kernel_operator.h`
- `FusedMulAdd<T>(dst, src0, src1, count)`: **dst[i] = src0[i] * dst[i] + src1[i]** (accumulates into dst, dst must be preset first).
- Example: Duplicate presets dst=1, src0=2, src1=3 -> 2*1+3 = 5. errors=0.
