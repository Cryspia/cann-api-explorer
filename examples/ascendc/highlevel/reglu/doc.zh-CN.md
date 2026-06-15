# Ascend C · ReGlu
- highlevel / 门控激活　- `AscendC::ReGlu`　- include：`lib/activation/reglu.h`
- `ReGlu<T>(dst, src0, src1, count)`：**dst = src0 \* relu(src1) = src0 \* max(0, src1)**。
- 例：src0=2，src1=3 -> dst = 2\*max(0,3) = 6.0。errors=0。
- 说明：adv_api 门控算子，本 SOC（NPU arch 3510）可用。
