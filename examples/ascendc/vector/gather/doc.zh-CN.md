# Ascend C · Gather
- 分类：矢量 / 收集　- API：`AscendC::Gather`　- include：`kernel_operator.h`
- `Gather<T>(dst, src, srcOffset, srcBaseAddr, count)`：dst[i]=src[(srcBaseAddr+srcOffset[i])/sizeof(T)]。
- **srcOffset 为字节偏移**。例：srcOffset[i]=(N-1-i)*4 → dst 逆序 [63,62,..,0]。errors=0（instr≈333）。
