# Ascend C · CreateVecIndex
- 分类：矢量 / 索引生成　- API：`AscendC::CreateVecIndex`　- include：`kernel_operator.h`
- `CreateVecIndex<T>(dst, firstValue, count)`：dst[i]=firstValue+i（等差索引，常用于配合 Gather/Scatter）。
- 例：firstValue=0, N=64 → dst[i]=i。errors=0（instr≈212）。
