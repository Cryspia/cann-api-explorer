# Ascend C · Compare
- vector / comparison　- `AscendC::Compare` (+ Select for verification)　- include: `kernel_operator.h`
- `Compare<T,U>(maskDst, src0, src1, CMPMODE, count)`: per-element comparison of two vectors, each bit of maskDst = the result. Differs from the select unit's CompareScalar (scalar comparison).
- Example: src0[i]=i, src1=31.5, LT -> mask=(i<32), Select converts to float to verify z[i]=(i<32)?1:0. errors=0 (instr≈410).
