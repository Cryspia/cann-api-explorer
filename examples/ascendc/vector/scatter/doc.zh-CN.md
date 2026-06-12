# Ascend C · Scatter
- 分类：矢量 / 散射　- API：`AscendC::Scatter`　- include：`kernel_operator.h`
- `Scatter<T>(dst, src, dstOffset, dstBaseAddr, count)`：dst[(dstBaseAddr+dstOffset[i])/sizeof(T)]=src[i]。
- **dstOffset 为字节偏移**（Gather 的逆操作）。例：dstOffset[i]=(N-1-i)*4 → dst 逆序。errors=0（instr≈333）。
