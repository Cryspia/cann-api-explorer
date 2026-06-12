# Ascend C · Compare
- 矢量 / 比较　- `AscendC::Compare`（+ Select 验证）　- include：`kernel_operator.h`
- `Compare<T,U>(maskDst, src0, src1, CMPMODE, count)`：两向量逐元素比较，maskDst 每 bit=结果。区别于 select 单元的 CompareScalar（标量比较）。
- 例：src0[i]=i, src1=31.5, LT → mask=(i<32)，Select 转 float 校验 z[i]=(i<32)?1:0。errors=0（instr≈410）。
