# Ascend C · Brcb（block broadcast）
- 分类：矢量 / 广播　- API：`AscendC::Brcb` + `BrcbRepeatParams`　- include：`kernel_operator.h`
- `Brcb<T>(dst, src0, repeatTime, {dstBlkStride, dstRepStride})`：一个 repeat 取 src 8 个 b32 元素，各广播成一个 32B block(8 float)，连续写 8 个 block。
- 例：src[8]=[0..7], repeatTime=1, dstBlkStride=1, dstRepStride=8 → dst[b*8+k]=src[b]（64 元素）。errors=0（instr≈275）。
