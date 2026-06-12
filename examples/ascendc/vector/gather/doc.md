# Ascend C · Gather
- Category: vector / gather　- API: `AscendC::Gather`　- include: `kernel_operator.h`
- `Gather<T>(dst, src, srcOffset, srcBaseAddr, count)`: dst[i]=src[(srcBaseAddr+srcOffset[i])/sizeof(T)].
- **srcOffset is a byte offset**. Example: srcOffset[i]=(N-1-i)*4 -> dst in reverse order [63,62,..,0]. errors=0 (instr≈333).
