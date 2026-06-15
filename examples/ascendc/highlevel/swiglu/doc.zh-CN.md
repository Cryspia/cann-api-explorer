# Ascend C · SwiGLU
- highlevel / 门控激活　- `AscendC::SwiGLU`　- include：`lib/activation/swiglu.h`
- `SwiGLU<T>(dst, src0, src1, beta, count)`：**dst = src0 \* silu_beta(src1)**，其中 `silu_beta(x) = x / (1 + exp(-beta*x))`。
- 例：src0=4，src1=3，beta=0 -> `silu_0(x)=x/2` -> dst = 4\*3/2 = 6.0。errors=0。
- 说明：adv_api 门控算子，本 SOC（NPU arch 3510）可用。
