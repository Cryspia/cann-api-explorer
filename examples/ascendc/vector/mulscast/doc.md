# Ascend C · MulsCast
- vector / fused multiply-by-scalar + type conversion　- `AscendC::MulsCast`　- include: `kernel_operator.h`
- `MulsCast(dst, src0, src1, count)`: **dst[i] = (DstType)(src0[i] * src1)**, where src1 is a scalar.
- Only supported dtype combination on this SoC: src=`float`, scalar=`float`, dst=`half`.
- Example: src0[i]=i, scalar=0.5 -> dst[i]=half(0.5*i). tol 5e-3, errors=0.
