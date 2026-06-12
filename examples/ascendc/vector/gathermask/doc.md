# Ascend C · GatherMask (stream compaction)
- vector / mask gather　- `AscendC::GatherMask` + `GatherMaskParams`　- include: `kernel_operator.h`
- `GatherMask<T,mode>(dst, src0, src1Pattern(uint8), reduceMode, mask, GatherMaskParams, rsvdCnt)`: select elements by pattern and pack them contiguously; `rsvdCnt` returns the number selected.
- **Measured: pattern=1 = select even indices**: src[0..63] -> dst[i]=src[2i]=[0,2,4,..,62], rsvdCnt=32. errors=0 (instr≈354).
- GatherMask is also a primitive used internally by the TopK implementation.
