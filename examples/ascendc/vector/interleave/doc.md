# Ascend C · Interleave
- vector / interleave (3510-only)　- `AscendC::Interleave`　- include: `kernel_operator.h`
- `Interleave<T>(dst0, dst1, src0, src1, count)`: interleaves two source vectors element by element. The lower N/2 pairs go to dst0 as [a0,b0,a1,b1,...], the upper N/2 pairs go to dst1. `count` must be even. Supported dtypes include int8/16/32, half, float, bfloat16.
- Example: src0 = all 1.0, src1 = all 2.0 -> concat(dst0, dst1) = [1,2,1,2,...]. errors=0.
