# Ascend C · Mull
- vector / wide integer multiply (dual output)　- `AscendC::Mull`　- include: `kernel_operator.h`
- `Mull<T>(dst0, dst1, src0, src1, count)`: **dst0 = low 32 bits, dst1 = high 32 bits of src0 * src1** (T = int32_t / uint32_t).
- Example: src0=2, src1=3 -> product 6 -> dst0(low)=6, dst1(high)=0. Only the low half is verified. errors=0.
