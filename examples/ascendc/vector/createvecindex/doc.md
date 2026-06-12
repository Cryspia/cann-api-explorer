# Ascend C · CreateVecIndex
- Category: vector / index generation　- API: `AscendC::CreateVecIndex`　- include: `kernel_operator.h`
- `CreateVecIndex<T>(dst, firstValue, count)`: dst[i]=firstValue+i (arithmetic-progression index, often used together with Gather/Scatter).
- Example: firstValue=0, N=64 -> dst[i]=i. errors=0 (instr≈212).
