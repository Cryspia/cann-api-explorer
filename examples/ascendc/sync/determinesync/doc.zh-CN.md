# Ascend C · DetermineSync（确定性计算同步，⚠️ 仿真死锁 / 未做成单元）
- 分类：同步 sync/　- API：`InitDetermineComputeWorkspace` / `WaitPreBlock` / `NotifyNextBlock`　- include：`kernel_operator.h`
- 用途：保证多核按 blockIdx 顺序执行某段以得确定性结果。
- **实测问题**：本链式实现（核 i `WaitPreBlock` 等前序 → 写 chain → `NotifyNextBlock`，对核 0/末核加 `if` 跳过）在 CAModel 多核仿真下**死锁**：sim_exe 跑满 607% CPU、34 分钟不退出、占 7GB+ 内存。根因疑为 `if(idx>0)/if(idx<num-1)` 破坏了 Wait/Notify 的严格配对，某核在 `WaitPreBlock` 死等永不到来的通知；这套 API 可能要求**每核都成对调用** Wait+Notify（内部按 idx 自洽），不能按链首/链尾裁剪。
- 已移出计数（meta.json.unsupported），代码留存作反例参考。**多核同步范式已由 SyncAll（全屏障）+ IBSet/IBWait（一对一）两个单元充分覆盖**，此 API 不再单独补单元。
