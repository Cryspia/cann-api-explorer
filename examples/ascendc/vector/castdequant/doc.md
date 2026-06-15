# Ascend C · CastDequant (vector, cast + dequant)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: vector / cast + dequant
- Covered API: `CastDequant` (`CastDeq` is its deprecated alias)
- include: `kernel_operator.h`
- Source: CANN 9.1.0 Ascend C API Reference / CastDequant

## Functionality
`CastDequant` casts an integer source and dequantizes it with a scalar dequant scale.
The header notes that `CastDeq` has been superseded by `CastDequant` (same operation, old name).

For the `int32 -> half` path the 3510 impl computes
`float(src) * (1/131072) * g_deqValue * 131072`, where the `131072` factors cancel, leaving
`dst = src * g_deqValue` (half). The dequant scale is the **scalar `g_deqValue`** set via
`SetDeqScale(half)`. The `int16 -> int8/uint8` path instead uses the bit-packed `g_deqScale`
and is not used here.

## Measured signature
```cpp
template <typename T, typename U, bool isVecDeq = true, bool halfBlock = true>
void CastDequant(const LocalTensor<T>& dst, const LocalTensor<U>& src, const uint32_t count);
// CastDeq has the same signature; the header says to use CastDequant instead.
```
- Supported dtype combos: `src:int16 -> dst:int8/uint8`, and `src:int32 -> dst:half` (used here).

## Minimal example design
- `N = 256`, `deqScale = 2.0`. src all `3` (int32) → `dst = 3 * 2 = 6` (half).
- Host decodes half and verifies `== 6`. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
