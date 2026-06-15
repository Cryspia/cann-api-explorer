# Ascend C · ReduceProd
- highlevel / 连乘归约　- `AscendC::ReduceProd`　- include：`adv_api/reduce/reduce.h`
- `ReduceProd<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad)`：沿末（R）轴对所有元素求积。`srcShape = {dimA, dimR}`。
- 约束：本架构（3510）仅支持 `float`。
- 例：shape [1,64]，全 1.0 仅 `x[3]=2.0` -> 积 = 2.0 -> dst[0] = 2.0。errors=0。
