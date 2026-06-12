# Ascend C · SyncAll（多核同步屏障）

- 分类：同步 sync/
- 覆盖 API：`AscendC::SyncAll` + `GetBlockIdx` / `GetBlockNum`
- include：`kernel_operator.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SyncAll

## 功能
核间屏障：所有参与核都执行到 `SyncAll` 后才一起继续。两种重载：
- `SyncAll<isAIVOnly>(gmWorkspace, ubWorkspace, usedCores)`：**软件屏障**，用 GM 计数 + dcci 做 cache flush/invalidate，通用。
- `SyncAll<isAIVOnly, config>()`（3510/5102）：**硬件 FFTS** 跨核同步指令（`ffts_cross_core_sync`），无需 workspace。

本例用软件版（自带 GM 一致性处理，最稳）。

## 关键 1：真正多核要 launch blockDim>1
`ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, ...)` 的第一个参数即核数；kernel 内 `GetBlockIdx()` 返回 0..blockDim-1，`GetBlockNum()` 返回 blockDim。本机仿真核可用 20，本例用 8。

## 关键 2：核间 GM 通信必须用 DataCopy，不能用标量 SetValue
标量 `GlobalTensor::SetValue` 写 GM 会滞留 cache、不保证回写 DDR → host memcpy 读到旧值（实测 flag 全为初值 -1）。
改用 `DataCopy(UB→GM)`（走 MTE3，保证回写）写、`DataCopy(GM→UB)` 读；标量(`GetValue`)与搬运之间插 `PipeBarrier<PIPE_ALL>()` 定序。
SyncAll 软件屏障在屏障点对 GM 做 flush/invalidate，确保阶段2 读到其它核阶段1 的写入。

## 最简 example 设计（验证同步语义）
- launch 8 核：每核把 `idx+1` 经 DataCopy 写到第 idx 个 32B 槽（SLOT=8 int32 对齐）。
- `SyncAll(syncGm, ubWs, num)` 屏障。
- 每核 DataCopy 读回全部 8 槽求和 → `result[idx]`。
- 期望：`flag[i]=i+1`，且**所有核** `result[i]=1+..+8=36`（若屏障失效，某核会读到尚未写入的槽，sum≠36）。
- host 校验 errors=0（instr≈6367，8 核）。
