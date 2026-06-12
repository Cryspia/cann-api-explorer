# Ascend C · Add (half variant)
- vector / binary half　- `AscendC::Add`　- include: `kernel_operator.h`
- Verifies the vector compute path for the **half data type** (same API as the float version, only `using DT = half`).
- host uses minimal F2H/H2F encode/decode: a=1.0, b=2.0 -> c=3.0. errors=0 (instr≈336).
