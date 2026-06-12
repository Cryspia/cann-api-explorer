# Ascend C · SetAtomicAdd（GM 原子累加）
- 分类：原子 / 多核　- API：`AscendC::SetAtomicAdd` / `SetAtomicNone`　- include：`kernel_operator.h`
- `SetAtomicAdd<T>()` 把后续 `DataCopy(UB→GM)` 设为**原子累加**模式，`SetAtomicNone()` 恢复普通写。
- 多核累加场景：launch 8 核，每核 `SetAtomicAdd` 后 DataCopy 全 1.0 到同一 GM 区 → 各元素 = 8.0（**GM 须 host 清零**）。
- 例：errors=0（instr≈1760，8 核）。另有 SetAtomicMax/Min。
