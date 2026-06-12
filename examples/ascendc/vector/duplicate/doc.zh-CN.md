# Ascend C · Duplicate
- 分类：矢量 / 数据填充　- API：`AscendC::Duplicate`　- include：`kernel_operator.h`
- `Duplicate<T>(dst, scalarValue, count)`：dst[i]=scalarValue（count 模式，level 2）。
- 例：N=64，scalar=3.0 → dst 全 3.0。host 校验 errors=0（instr≈204）。
