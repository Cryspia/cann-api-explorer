# Ascend C · AbsSub
- 矢量 / 绝对差　- `AscendC::AbsSub`　- include：`kernel_operator.h`
- `AbsSub<T>(dst, src0, src1, count)`：**dst[i] = |src0[i] - src1[i]|**。
- 例：src0=2，src1=5 → dst = |2-5| = 3。errors=0。
