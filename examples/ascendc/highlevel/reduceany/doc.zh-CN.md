# Ascend C · ReduceAny
- highlevel / 逻辑“任一为真”归约　- `AscendC::ReduceAny`　- include：`adv_api/reduce/reduce.h`
- `ReduceAny<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`：沿末（R）轴做“任一元素为真”。`srcShape = {dimA, dimR}`。
- 约束：数据类型 `uint8_t`/`float`。对 `uint8_t` 结果为最大元素，因此任一非零 -> 非零，全零 -> 0。
- 例：shape [1,64]，全 0 仅 `x[7]=1` -> dst[0] = 1。errors=0。
