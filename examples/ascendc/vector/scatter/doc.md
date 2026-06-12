# Ascend C · Scatter
- Category: vector / scatter　- API: `AscendC::Scatter`　- include: `kernel_operator.h`
- `Scatter<T>(dst, src, dstOffset, dstBaseAddr, count)`: dst[(dstBaseAddr+dstOffset[i])/sizeof(T)]=src[i].
- **dstOffset is a byte offset** (the inverse of Gather). Example: dstOffset[i]=(N-1-i)*4 -> dst in reverse order. errors=0 (instr≈333).
