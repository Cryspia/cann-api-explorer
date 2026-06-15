# Ascend C · SwiGLU
- highlevel / gated activation　- `AscendC::SwiGLU`　- include: `lib/activation/swiglu.h`
- `SwiGLU<T>(dst, src0, src1, beta, count)`: **dst = src0 \* silu_beta(src1)**, where `silu_beta(x) = x / (1 + exp(-beta*x))`.
- Example: src0=4, src1=3, beta=0 -> `silu_0(x)=x/2` -> dst = 4\*3/2 = 6.0. errors=0.
- Note: adv_api gated op, available on this SOC (NPU arch 3510).
