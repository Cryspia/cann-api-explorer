# Ascend C · Duplicate
- Category: vector / data fill　- API: `AscendC::Duplicate`　- include: `kernel_operator.h`
- `Duplicate<T>(dst, scalarValue, count)`: dst[i]=scalarValue (count mode, level 2).
- Example: N=64, scalar=3.0 -> dst all 3.0. host verify errors=0 (instr≈204).
