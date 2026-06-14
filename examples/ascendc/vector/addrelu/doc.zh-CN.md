# Ascend C · AddRelu
- 矢量 / 融合加法 + ReLU　- `AscendC::AddRelu`　- include：`kernel_operator.h`
- `AddRelu<T>(dst, src0, src1, count)`：**dst[i] = max(src0[i] + src1[i], 0)**。
- 例：src0=1，src1=2 → dst = max(3, 0) = 3。errors=0。
