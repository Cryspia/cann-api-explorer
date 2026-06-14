# Ascend C · MulAddRelu
- vector / fused multiply-add + ReLU　- `AscendC::MulAddRelu`　- include: `kernel_operator.h`
- `MulAddRelu<T>(dst, src0, src1, count)`: **dst[i] = max(src0[i] * dst[i] + src1[i], 0)** (accumulates into dst then ReLU, dst must be preset first).
- Example: Duplicate presets dst=1, src0=2, src1=3 -> relu(2*1+3) = 5. errors=0.
