# Ascend C · Broadcast（高阶，device 端算 Tiling）

- 分类：高阶 adv_api / 广播
- 覆盖 API：`AscendC::Broadcast` + `AscendC::GetBroadcastTilingInfo`
- include：`lib/pad/broadcast.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / Broadcast（numpy broadcasting 语义）

## 功能
按 numpy 广播规则把 `srcShape` 扩展到 `dstShape`：某维 `src=1, dst=M` 则沿该维复制 M 份。
本例 `[1,8] → [4,8]`（沿 axis0 复制行）。

## 关键：3510 在 kernel 内算 tiling（device-side TilingFunc）
3510/5102/3003/3113 提供 `GetBroadcastTilingInfo`，在**核函数内**算出 `BroadcastTiling`，无需 host 计算、也无需手填：
```cpp
uint32_t srcShape[2] = {1, N};
uint32_t dstShape[2] = {M, N};
AscendC::BroadcastTiling tiling;
AscendC::GetBroadcastTilingInfo<float>(2, dstShape, srcShape, /*srcInnerPad*/false, tiling);
AscendC::Broadcast<float>(zL, xL, dstShape, srcShape, &tiling);
```
- `Broadcast`（3510 重载）不收 `sharedTmpBuffer`，内部用 `PopStackBuffer<T,TPosition::LCM>` 从 TPipe 剩余 UB 取临时空间——数据量小，UB 充足即可。
- `srcInnerPad`：src 末维是否需内部对齐填充；末维 `N=8`（32B 对齐）取 `false`。
- 这是与 SoftMax 同类的「device 端 *TilingFunc 在核内」技术，区别于 TopK 的 host-tiling、Pad 的手填。

## 最简 example 设计
- `src=[0..7]`（`[1,8]`），`dst=[4,8]`，期望每行 `0..7`。单核，host 校验 errors=0（instr≈620）。
