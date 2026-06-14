# Ascend C · MulAddDst
- vector / fused multiply-add (into dst)　- `AscendC::MulAddDst`　- include: `kernel_operator.h`
- `MulAddDst<T>(dst, src0, src1, count)`: **dst[i] = src0[i] * src1[i] + dst[i]** (accumulates into dst, dst must be preset first).
- Example: Duplicate presets dst=1, src0=2, src1=3 -> 2*3+1 = 7. errors=0.
