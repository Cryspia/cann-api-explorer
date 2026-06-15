# Ascend C · Interleave
- 分类：矢量 / 交错（仅 3510）　- API：`AscendC::Interleave`　- include：`kernel_operator.h`
- `Interleave<T>(dst0, dst1, src0, src1, count)`：把两路源向量逐元素交错。前 N/2 对写入 dst0，形如 [a0,b0,a1,b1,...]；后 N/2 对写入 dst1。`count` 必须为偶数。支持 int8/16/32、half、float、bfloat16 等。
- 例：src0 全为 1.0，src1 全为 2.0 → concat(dst0, dst1) = [1,2,1,2,...]。errors=0。
