# Ascend C · Truncate
- vector / conversion (3510-only)　- `AscendC::Truncate`　- include: `kernel_operator.h`
- `Truncate<T, roundMode>(dst, src, count)`: rounds each element to an integral value while keeping the **same** floating type (half / float / bfloat16). Supported roundModes: CAST_RINT, CAST_FLOOR, CAST_CEIL, CAST_ROUND, CAST_TRUNC. This is NOT a dtype-changing cast (unlike `Cast`) and NOT the math `Trunc` helper — it is the hardware rounding conversion in float form.
- Example: roundMode = CAST_TRUNC (round toward zero), src = alternating +/-(k+0.9) -> +/-k.0. errors=0.
