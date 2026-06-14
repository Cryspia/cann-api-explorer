# Ascend C · MulAddRelu
- 矢量 / 融合乘加 + ReLU　- `AscendC::MulAddRelu`　- include：`kernel_operator.h`
- `MulAddRelu<T>(dst, src0, src1, count)`：**dst[i] = max(src0[i] * dst[i] + src1[i], 0)**（累加到 dst 后 ReLU，dst 须先预置）。
- 例：Duplicate 预置 dst=1，src0=2，src1=3 → relu(2*1+3) = 5。errors=0。
