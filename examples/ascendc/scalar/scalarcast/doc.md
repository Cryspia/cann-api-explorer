# Ascend C · ScalarCast (scalar cast)
- Category: scalar　- API: `AscendC::ScalarCast` (for new code, the same-signature scalar `Cast` is recommended)　- include: `kernel_operator.h`
- `ScalarCast<T, U, RoundMode>(valueIn)`: scalar T->U, rounded per RoundMode (CAST_RINT round to nearest / CAST_TRUNC truncate / CAST_FLOOR / CAST_CEIL).
- Example: `ScalarCast<float,int32_t,CAST_RINT>(3.7f)` -> 4, Duplicate fills GM for verification. errors=0 (instr approx 204).
