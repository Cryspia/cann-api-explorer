# Ascend C · Pad (high-level, 3 hand-filled Tiling fields)

- Category: high-level adv_api / padding
- APIs covered: `AscendC::Pad` (together with `PadParams` / `PadTiling`)
- include: `lib/pad/pad.h`
- Source: CANN 9.1.0 Ascend C API Reference / Pad

## Function
`Pad<T>(dstTensor, srcTensor, PadParams&, sharedTmpBuffer, PadTiling&)`: on the right side of each row of `[srcHeight, srcWidth]`
(the aligned path only right-pads) it fills in `rightPad` copies of `padValue`. `PadParams{uint16 leftPad, uint16 rightPad, int32 padValue}`.

## Key point: the 3510 impl only reads 3 tiling fields
`PadTiling` has 20+ fields, but this arch (3510)'s `PadImpl` only references `srcHeight / srcWidth / srcOriWidth`,
so they can be hand-filled inside the kernel, avoiding host tiling (same approach as RmsNorm/GroupNorm):
```cpp
AscendC::PadParams params(/*leftPad*/0, /*rightPad*/4, /*padValue*/7);
AscendC::tiling::PadTiling t;
t.srcHeight = 1; t.srcWidth = 16; t.srcOriWidth = 12;
AscendC::Pad<float>(zL, xL, params, tmp, t);
```

## Which path it takes (aligned / unaligned)
- `PadCompute` checks `srcWidth * sizeof(T) % 32 == 0` -> `AlignedPad`, otherwise `UnAlignedPad` (the latter is the one that handles leftPad).
- `regBlockElementCnt = CUBE_MAX_SIZE/sizeof(T)`, CUBE_MAX_SIZE=256 -> **64** for float.
- Picking `srcWidth=16 < 64`: a single reg block, `regBlockCntPerRow=0`, the simplest logic.
- `rightPadMask` covers the interval `[srcOriWidth, srcOriWidth+rightPad)`; setting `srcOriWidth+rightPad = srcWidth` makes the tail fill exactly, with no garbage region.

## Simplest example design
- `[H=1, W=16]`, `srcOriWidth=12`, `rightPad=4`, `padValue=7`, `src[i]=i+1`.
- Expect `dst=[1..12, 7,7,7,7]`. Single core, host verify errors=0 (instr≈318).
