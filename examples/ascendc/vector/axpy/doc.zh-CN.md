# Ascend C · Axpy
- 矢量 / 融合乘加　- `AscendC::Axpy`　- include：`kernel_operator.h`
- `Axpy<T,U>(dst, src, scalarValue, count)`：**dst[i] += scalarValue*src[i]**（累加到 dst，dst 须先预置）。
- 例：Duplicate 预置 dst=1，src[i]=i，scalar=2 → dst[i]=1+2i。errors=0（instr≈289）。
