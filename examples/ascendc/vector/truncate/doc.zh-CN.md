# Ascend C · Truncate
- 分类：矢量 / 转换（仅 3510）　- API：`AscendC::Truncate`　- include：`kernel_operator.h`
- `Truncate<T, roundMode>(dst, src, count)`：把每个元素按 roundMode 取整为整数值，但保持**同一**浮点类型（half / float / bfloat16）。支持 roundMode：CAST_RINT、CAST_FLOOR、CAST_CEIL、CAST_ROUND、CAST_TRUNC。它不是改变 dtype 的 `Cast`，也不是数学库的 `Trunc`，而是以 float 形式输出的硬件取整转换。
- 例：roundMode = CAST_TRUNC（向零取整），src 为交替的 +/-(k+0.9) → +/-k.0。errors=0。
