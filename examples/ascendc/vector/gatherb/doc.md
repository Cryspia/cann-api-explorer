# Ascend C · Gatherb
- vector / gather (block-level)　- `AscendC::Gatherb`　- include: `kernel_operator.h`
- `Gatherb<T>(dst, src0, offset, repeatTime, GatherRepeatParams)`: for each offset it fetches one 32-byte block (= 8 floats) from src0 at the given **byte** offset and writes the blocks contiguously into dst. One repeat consumes 8 offsets (DEFAULT_BLK_NUM) -> 8 blocks. This is block granularity, unlike the element-wise `Gather`.
- Example: src[i]=i (64 floats = 8 blocks), offset[k]=(8-1-k)*32 bytes -> dst has the 8 blocks reversed: dst[8*b+j]=(7-b)*8+j. errors=0.
