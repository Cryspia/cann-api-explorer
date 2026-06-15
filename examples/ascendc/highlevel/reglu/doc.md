# Ascend C · ReGlu
- highlevel / gated activation　- `AscendC::ReGlu`　- include: `lib/activation/reglu.h`
- `ReGlu<T>(dst, src0, src1, count)`: **dst = src0 \* relu(src1) = src0 \* max(0, src1)**.
- Example: src0=2, src1=3 -> dst = 2\*max(0,3) = 6.0. errors=0.
- Note: adv_api gated op, available on this SOC (NPU arch 3510).
