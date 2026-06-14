# Ascend C · FusedMulAdd
- 矢量 / 融合乘加　- `AscendC::FusedMulAdd`　- include：`kernel_operator.h`
- `FusedMulAdd<T>(dst, src0, src1, count)`：**dst[i] = src0[i] * dst[i] + src1[i]**（累加到 dst，dst 须先预置）。
- 例：Duplicate 预置 dst=1，src0=2，src1=3 → 2*1+3 = 5。errors=0。
