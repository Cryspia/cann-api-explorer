# Ascend C · Ors
- vector / element-wise bitwise (tensor + scalar)　- `AscendC::Ors`　- include: `kernel_operator.h`
- `Ors(dst, src0, src1, count)`: **dst[i] = src0[i] | src1**, where src1 is a scalar.
- Level-2 supported integer dtypes on this SoC: `int16`/`uint16`/`int64`/`uint64` (`int32` is NOT supported), so this unit uses `int16`.
- Example: src0[i]=6, scalar=3 -> dst[i]=7. errors=0.
