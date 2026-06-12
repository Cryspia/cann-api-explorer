# Ascend C · Cast (float->half)
- vector / type conversion　- `AscendC::Cast`　- include: `kernel_operator.h`
- `Cast(dst, src, RoundMode, count)`: **RoundMode is a runtime parameter (the 3rd one), not a template parameter** (pitfall). DST=half, SRC=float, CAST_NONE.
- Complements the existing float->int32 cast unit. x[i]=i (0..255, exact in half) -> z=(half)i. errors=0 (instr≈301).
