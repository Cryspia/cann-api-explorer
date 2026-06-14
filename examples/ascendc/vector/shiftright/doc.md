# Ascend C · ShiftRight
- vector / element-wise bitwise　- `AscendC::ShiftRight`　- include: `kernel_operator.h`
- `ShiftRight<T,U>(dst, src0, src1, count)`: **dst[i] = src0[i] >> src1[i]** (both src0 and src1 are tensors).
- Supported dtype combinations on this SoC: src0/dst `int32`+shift `int32`, also `int64`/`uint64`/`uint32`.
- Example: src0[i]=16, src1[i]=2 -> dst[i]=4. errors=0.
