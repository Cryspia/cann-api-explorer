# Ascend C · VectorPadding（⚠️ 未做成单元 / 语义待深挖）
- 矢量 / 边界填充　- `AscendC::VectorPadding`　- include：`kernel_operator.h`
- `VectorPadding(dst, src, padMode, padSide, count)`。
- **实测 count 模式 padMode=0 输出全 0**，难解释为有意义的边界填充——其语义疑依赖 mask+repeat（level 0）模式与 padMode 的精确定义，count 简化模式不足以表达。
- 已移出计数（meta.json.unsupported）。接口代码留存作参考。与 adv_api `Pad`（已覆盖）不同层。
