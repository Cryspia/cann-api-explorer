# Ascend C · Pad（高阶，3 字段手填 Tiling）

- 分类：高阶 adv_api / 填充
- 覆盖 API：`AscendC::Pad`（配套 `PadParams` / `PadTiling`）
- include：`lib/pad/pad.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / Pad

## 功能
`Pad<T>(dstTensor, srcTensor, PadParams&, sharedTmpBuffer, PadTiling&)`：对 `[srcHeight, srcWidth]` 的每行右侧
（对齐路径仅右填充）填入 `rightPad` 个 `padValue`。`PadParams{uint16 leftPad, uint16 rightPad, int32 padValue}`。

## 关键：3510 impl 只读 3 个 tiling 字段
`PadTiling` 有 20+ 字段，但本 arch(3510) 的 `PadImpl` 只引用 `srcHeight / srcWidth / srcOriWidth`，
故可在 kernel 内手填、免去 host tiling（与 RmsNorm/GroupNorm 同套路）：
```cpp
AscendC::PadParams params(/*leftPad*/0, /*rightPad*/4, /*padValue*/7);
AscendC::tiling::PadTiling t;
t.srcHeight = 1; t.srcWidth = 16; t.srcOriWidth = 12;
AscendC::Pad<float>(zL, xL, params, tmp, t);
```

## 走哪条路径（对齐 / 非对齐）
- `PadCompute` 判 `srcWidth * sizeof(T) % 32 == 0` → `AlignedPad`，否则 `UnAlignedPad`（后者才处理 leftPad）。
- `regBlockElementCnt = CUBE_MAX_SIZE/sizeof(T)`，CUBE_MAX_SIZE=256 → float 为 **64**。
- 选 `srcWidth=16 < 64`：单个 reg block，`regBlockCntPerRow=0`，逻辑最简。
- `rightPadMask` 覆盖区间 `[srcOriWidth, srcOriWidth+rightPad)`，令 `srcOriWidth+rightPad = srcWidth` 时尾部正好填满，无垃圾区。

## 最简 example 设计
- `[H=1, W=16]`，`srcOriWidth=12`，`rightPad=4`，`padValue=7`，`src[i]=i+1`。
- 期望 `dst=[1..12, 7,7,7,7]`。单核，host 校验 errors=0（instr≈318）。
