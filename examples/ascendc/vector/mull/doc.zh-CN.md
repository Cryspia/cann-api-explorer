# Ascend C · Mull
- 矢量 / 宽整数乘法（双输出）　- `AscendC::Mull`　- include：`kernel_operator.h`
- `Mull<T>(dst0, dst1, src0, src1, count)`：**dst0 = src0 * src1 的低 32 位，dst1 = 高 32 位**（T = int32_t / uint32_t）。
- 例：src0=2，src1=3 → 乘积 6 → dst0(低)=6，dst1(高)=0。仅校验低位。errors=0。
