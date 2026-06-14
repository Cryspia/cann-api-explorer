# Ascend C · ShiftRight
- 矢量 / 逐元素位运算　- `AscendC::ShiftRight`　- include：`kernel_operator.h`
- `ShiftRight<T,U>(dst, src0, src1, count)`：**dst[i] = src0[i] >> src1[i]**（src0、src1 均为 tensor）。
- 本 SoC 支持的 dtype 组合：src0/dst 为 `int32` + 移位量 `int32`，另支持 `int64`/`uint64`/`uint32`。
- 例：src0[i]=16，src1[i]=2 → dst[i]=4。errors=0。
