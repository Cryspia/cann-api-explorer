# Ascend C · Compares
- 分类：矢量 / 比较（向量与标量）　- API：`AscendC::Compares`（配合 Select 校验）　- include：`kernel_operator.h`
- `Compares<T,U>(maskDst, src0, src1Scalar, CMPMODE, count)`：把向量每个元素与一个**标量**比较，结果写入位 mask。`Compares` 是当前名字；头文件标注 `CompareScalar` 已废弃（"CompareScalar has been updated, please use Compares instead"）——二者标量比较语义相同，但 `Compares` 是规范 API。区别于 `Compare`（向量-向量逐元素比较）。
- 例：src0[i]=i，标量=31.5，GT → mask=(i>=32)；Select 转为 float z[i]=(i>=32)?1:0。errors=0。
