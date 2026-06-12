# Ascend C · Brcb (block broadcast)
- Category: vector / broadcast　- API: `AscendC::Brcb` + `BrcbRepeatParams`　- include: `kernel_operator.h`
- `Brcb<T>(dst, src0, repeatTime, {dstBlkStride, dstRepStride})`: one repeat takes 8 b32 elements from src, broadcasts each into a 32B block (8 float), writing 8 blocks contiguously.
- Example: src[8]=[0..7], repeatTime=1, dstBlkStride=1, dstRepStride=8 -> dst[b*8+k]=src[b] (64 elements). errors=0 (instr≈275).
