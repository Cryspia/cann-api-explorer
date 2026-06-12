# Ascend C · GatherMask（流压缩）
- 矢量 / 掩码收集　- `AscendC::GatherMask` + `GatherMaskParams`　- include：`kernel_operator.h`
- `GatherMask<T,mode>(dst, src0, src1Pattern(uint8), reduceMode, mask, GatherMaskParams, rsvdCnt)`：按 pattern 选元素紧凑排列，`rsvdCnt` 返回选中数。
- **实测 pattern=1 = 选偶数索引**：src[0..63] → dst[i]=src[2i]=[0,2,4,..,62]，rsvdCnt=32。errors=0（instr≈354）。
- GatherMask 也是 TopK 内部实现用到的原语。
