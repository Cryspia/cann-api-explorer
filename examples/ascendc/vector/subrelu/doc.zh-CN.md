# Ascend C · SubRelu
- 矢量 / 融合减法 + ReLU　- `AscendC::SubRelu`　- include：`kernel_operator.h`
- `SubRelu<T>(dst, src0, src1, count)`：**dst[i] = max(src0[i] - src1[i], 0)**。
- 例：src0=5，src1=2 → dst = max(3, 0) = 3。errors=0。
