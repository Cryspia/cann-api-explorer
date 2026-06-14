# Ascend C · ExpSub
- vector / exp of difference　- `AscendC::ExpSub`　- include: `kernel_operator.h`
- `ExpSub<T,U>(dst, src0, src1, count)`: **dst[i] = exp(src0[i] - src1[i])** (src half/float, dst float).
- Example: src0=1, src1=1 -> dst = exp(0) = 1 (tol 1e-2). errors=0.
