# Ascend C · DeInterleave
- vector / deinterleave (3510-only)　- `AscendC::DeInterleave`　- include: `kernel_operator.h`
- `DeInterleave<T>(dst0, dst1, src, srcCount)`: inverse of Interleave (single-source form). Splits an interleaved stream src = [a0,b0,a1,b1,...] of length `srcCount` (even) into dst0 = [a0,a1,...] (even positions) and dst1 = [b0,b1,...] (odd positions), each of length srcCount/2. A two-source form `DeInterleave(dst0,dst1,src0,src1,count)` also exists.
- Example: src[i]=i -> dst0=[0,2,4,...], dst1=[1,3,5,...]. errors=0.
