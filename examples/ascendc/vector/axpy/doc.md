# Ascend C · Axpy
- vector / fused multiply-add　- `AscendC::Axpy`　- include: `kernel_operator.h`
- `Axpy<T,U>(dst, src, scalarValue, count)`: **dst[i] += scalarValue*src[i]** (accumulates into dst, dst must be preset first).
- Example: Duplicate presets dst=1, src[i]=i, scalar=2 -> dst[i]=1+2i. errors=0 (instr≈289).
