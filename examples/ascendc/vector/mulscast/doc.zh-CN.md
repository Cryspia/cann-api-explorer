# Ascend C · MulsCast
- 矢量 / 融合“乘标量 + 类型转换”　- `AscendC::MulsCast`　- include：`kernel_operator.h`
- `MulsCast(dst, src0, src1, count)`：**dst[i] = (目标类型)(src0[i] * src1)**，其中 src1 为标量。
- 本 SoC 仅支持一种 dtype 组合：src=`float`，标量=`float`，dst=`half`。
- 例：src0[i]=i，标量=0.5 → dst[i]=half(0.5*i)。容差 5e-3，errors=0。
