# Ascend C · Add（half 变体）
- 矢量 / binary half　- `AscendC::Add`　- include：`kernel_operator.h`
- 验证 **half 数据类型**的矢量计算路径（与 float 版同 API，仅 `using DT = half`）。
- host 用 F2H/H2F 极简编解码：a=1.0, b=2.0 → c=3.0。errors=0（instr≈336）。
