# Ascend C · UnPad (high-level, pad)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / pad
- Covered API: `UnPad` (the inverse of `Pad`)
- include: `lib/pad/pad.h`
- Source: CANN 9.1.0 Ascend C API Reference / UnPad

## Functionality
`UnPad` is the inverse operation of `Pad`: it removes padding columns from the right of
each row. Where `Pad` appends `rightPad` elements per row, `UnPad` drops `rightPad`
elements per row, producing a row of `srcWidth - rightPad` valid elements.

## Measured signature
```cpp
template <typename T>
void UnPad(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor,
           UnPadParams& unPadParams, LocalTensor<uint8_t>& sharedTmpBuffer, UnPadTiling& tiling);
// also a no-tmp overload that pops a stack buffer internally.
```
- `UnPadParams(leftPad, rightPad)` — both `uint16_t`. The 3510 impl reads `rightPad` only;
  `leftPad` is ignored on this architecture (kept for symmetry with `Pad`).
- `UnPadTiling` — the 3510 impl reads `srcHeight` and `srcWidth`. It copies
  `srcWidth - rightPad` valid elements per row from src to dst.
- `T` supports `int16_t/uint16_t/half/int32_t/uint32_t/float`.

## Minimal example design
- Single row, `srcWidth = 16` (16*4 = 64B, 32B aligned), `rightPad = 4`.
- `src = [1..16]` → `dst[0..11] = [1..12]`; the last 4 padding columns are dropped.
- This mirrors the existing `pad` unit (`src=[1..16]`, `rightPad=4`, pad value appended).
- Host verifies the 12 valid elements exactly. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
