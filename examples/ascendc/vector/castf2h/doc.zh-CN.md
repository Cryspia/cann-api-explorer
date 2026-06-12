# Ascend C · Cast（float→half）
- 矢量 / 类型转换　- `AscendC::Cast`　- include：`kernel_operator.h`
- `Cast(dst, src, RoundMode, count)`：**RoundMode 是运行时参数（第3个），非模板参数**（踩坑点）。DST=half, SRC=float, CAST_NONE。
- 补已有 float→int32 的 cast 单元。x[i]=i（0..255 half 精确）→ z=(half)i。errors=0（instr≈301）。
