# Ascend C · Gatherb
- 分类：矢量 / 收集（按 block）　- API：`AscendC::Gatherb`　- include：`kernel_operator.h`
- `Gatherb<T>(dst, src0, offset, repeatTime, GatherRepeatParams)`：对每个 offset，从 src0 的该**字节**偏移处取一个 32 字节 block（= 8 个 float），按顺序写入 dst。一次 repeat 消费 8 个 offset（DEFAULT_BLK_NUM）→ 取 8 个 block。粒度为 block，区别于逐元素的 `Gather`。
- 例：src[i]=i（64 个 float = 8 个 block），offset[k]=(8-1-k)*32 字节 → dst 把 8 个 block 逆序：dst[8*b+j]=(7-b)*8+j。errors=0。
