# Ascend C · MulAddDst
- 矢量 / 融合乘加（累加到 dst）　- `AscendC::MulAddDst`　- include：`kernel_operator.h`
- `MulAddDst<T>(dst, src0, src1, count)`：**dst[i] = src0[i] * src1[i] + dst[i]**（累加到 dst，dst 须先预置）。
- 例：Duplicate 预置 dst=1，src0=2，src1=3 → 2*3+1 = 7。errors=0。
