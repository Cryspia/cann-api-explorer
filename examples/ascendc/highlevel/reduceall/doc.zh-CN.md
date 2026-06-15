# Ascend C · ReduceAll
- highlevel / 逻辑“全部为真”归约　- `AscendC::ReduceAll`　- include：`adv_api/reduce/reduce.h`
- `ReduceAll<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`：沿末（R）轴做“全部元素为真”。`srcShape = {dimA, dimR}`。
- 约束：数据类型 `uint8_t`/`float`。对 `uint8_t` 结果为最小元素，因此全部非零 -> 非零，任一为零 -> 0。
- 例：shape [1,64]，全 1 -> dst[0] = 1。errors=0。
