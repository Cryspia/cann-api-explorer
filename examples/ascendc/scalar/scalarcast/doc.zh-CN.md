# Ascend C · ScalarCast（标量类型转换）
- 分类：标量　- API：`AscendC::ScalarCast`（新代码建议用同签名的标量 `Cast`）　- include：`kernel_operator.h`
- `ScalarCast<T, U, RoundMode>(valueIn)`：标量 T→U，按 RoundMode 舍入（CAST_RINT 四舍五入 / CAST_TRUNC 截断 / CAST_FLOOR / CAST_CEIL）。
- 例：`ScalarCast<float,int32_t,CAST_RINT>(3.7f)` → 4，Duplicate 写满 GM 校验。errors=0（instr≈204）。
