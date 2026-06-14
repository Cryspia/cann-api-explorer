# Ascend C · Ands
- 矢量 / 逐元素位运算（tensor + 标量）　- `AscendC::Ands`　- include：`kernel_operator.h`
- `Ands(dst, src0, src1, count)`：**dst[i] = src0[i] & src1**，其中 src1 为标量。
- 本 SoC Level-2 支持的整型：`int16`/`uint16`/`int64`/`uint64`（**不支持** `int32`），故本单元使用 `int16`。
- 例：src0[i]=6，标量=3 → dst[i]=2。errors=0。
