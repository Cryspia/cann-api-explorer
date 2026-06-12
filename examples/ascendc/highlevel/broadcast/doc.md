# Ascend C · Broadcast (high-level, Tiling computed device-side)

- Category: high-level adv_api / broadcast
- Covered API: `AscendC::Broadcast` + `AscendC::GetBroadcastTilingInfo`
- include: `lib/pad/broadcast.h`
- Source: CANN 9.1.0 Ascend C API Reference / Broadcast (numpy broadcasting semantics)

## Functionality
Following numpy broadcasting rules, expand `srcShape` to `dstShape`: for a dimension where `src=1, dst=M`, replicate M copies along that dimension.
This example does `[1,8] -> [4,8]` (replicate rows along axis0).

## Key point: 3510 computes tiling inside the kernel (device-side TilingFunc)
3510/5102/3003/3113 provide `GetBroadcastTilingInfo`, which computes `BroadcastTiling` **inside the kernel**, with no host-side computation and no hand-filled tiling needed:
```cpp
uint32_t srcShape[2] = {1, N};
uint32_t dstShape[2] = {M, N};
AscendC::BroadcastTiling tiling;
AscendC::GetBroadcastTilingInfo<float>(2, dstShape, srcShape, /*srcInnerPad*/false, tiling);
AscendC::Broadcast<float>(zL, xL, dstShape, srcShape, &tiling);
```
- `Broadcast` (the 3510 overload) does not take `sharedTmpBuffer`; internally it uses `PopStackBuffer<T,TPosition::LCM>` to obtain temporary space from the remaining UB of the TPipe -- small data volume, so sufficient UB is enough.
- `srcInnerPad`: whether the last dimension of src needs internal alignment padding; with last dimension `N=8` (32B aligned), use `false`.
- This is the same "device-side *TilingFunc inside the kernel" technique as SoftMax, distinct from TopK's host-tiling and Pad's hand-filled tiling.

## Minimal example design
- `src=[0..7]` (`[1,8]`), `dst=[4,8]`, expecting every row to be `0..7`. Single core, host verify errors=0 (instr~620).
