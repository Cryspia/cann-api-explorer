# Ascend C · DeInterleave
- 分类：矢量 / 解交错（仅 3510）　- API：`AscendC::DeInterleave`　- include：`kernel_operator.h`
- `DeInterleave<T>(dst0, dst1, src, srcCount)`：Interleave 的逆操作（单源形式）。把交错流 src = [a0,b0,a1,b1,...]（长度 `srcCount`，偶数）拆回 dst0 = [a0,a1,...]（偶数位）与 dst1 = [b0,b1,...]（奇数位），各长 srcCount/2。另有双源形式 `DeInterleave(dst0,dst1,src0,src1,count)`。
- 例：src[i]=i → dst0=[0,2,4,...]，dst1=[1,3,5,...]。errors=0。
