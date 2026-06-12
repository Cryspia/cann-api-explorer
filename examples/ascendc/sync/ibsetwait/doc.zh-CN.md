# Ascend C · IBSet / IBWait（跨核一对一 flag 同步）

- 分类：同步 sync/
- 覆盖 API：`AscendC::IBSet` / `AscendC::IBWait` + `GetBlockIdx` / `GetBlockNum`
- include：`kernel_operator.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / IBSet、IBWait

## 功能
比 `SyncAll`（全核屏障）更细粒度的**一对一/生产者-消费者**跨核同步。两者操作**同一 GM 槽** `gmWorkspace[blockNum*8*eventID + blockIdx*8]`（每核每 event 占 32B）：
- `IBSet<isAIVOnly>(gmWorkspace, ubWorkspace, blockIdx, eventID)`：轮询该槽，`==0` 时置 `1`（置位，宣告「我完成了」）。
- `IBWait<isAIVOnly>(gmWorkspace, ubWorkspace, blockIdx, eventID)`：轮询该槽，`==1` 时置 `0`（消费，等到对应核置位）。
- 配对靠 `blockIdx`（槽编号，约定用生产者核 idx）；`eventID` 区分多个独立事件。

## 关键
- 必须 launch 多核（`ACLRT_LAUNCH_KERNEL(k)(blockDim,...)`），`gmWorkspace` 须 host 清零（初始 0）。
- `ubWorkspace` ≥ 32B（impl 内部 DataCopy 一个 32B 块轮询）。
- impl 内部自带 `pipe_barrier(PIPE_ALL)` + MTE2/MTE3 flag；核间数据（chain）仍用 `DataCopy` 读写 GM。
- 与 `SyncAll` 区别：SyncAll 是「所有核到齐」对称屏障；IBSet/IBWait 是「A 通知 B」有向同步，可搭出流水线/链式依赖。

## 最简 example 设计（链式累加，验证严格顺序）
- launch 8 核，链式 `0→1→…→7`：
  - 核 i：若 i>0 先 `IBWait(slot i-1)` 等核 i-1；读 `chain[i-1]` 写 `chain[i]=chain[i-1]+1`（核 0 前驱视为 0 → 写 1）；若 i<7 再 `IBSet(slot i)` 通知核 i+1。
- 期望 `chain[i]=i+1`（核 7=8）。**若 IBWait 失效，核 i 会读到前驱尚未写入的旧值（-1）→ 结果错**。
- host 校验 errors=0（instr≈7956，8 核）。
