# Ascend C · AscendQuant (high-level, quantization)

- Category: high-level adv_api / quantization
- Covered API: `AscendQuant` (per-tensor)
- include: `lib/quantization/ascend_quant.h`
- Source: CANN 9.1.0 Ascend C API Reference / AscendQuant

## Functionality
Per-tensor quantization: `dst(int8) = round(src * scale + offset)`.
`scale` and `offset` are **scalar floats** (per-tensor). A per-channel form (scale/offset
as `LocalTensor`) also exists but is not used here.

## Measured signature
```cpp
template <typename T, bool isReuseSource = false, const AscendQuantConfig& config = ASCEND_QUANT_DEFAULT_CFG>
void AscendQuant(const LocalTensor<int8_t>& dst, const LocalTensor<T>& src,
                 const LocalTensor<uint8_t>& sharedTmpBuffer,
                 const float scale, const float offset, const uint32_t calCount);
```
- `T` supports `half` / `float`; output is always `int8_t`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>` (sized per the tiling API; here over-allocated to 8 KB).

## Minimal example design
- `ELEM = 256`, src all `2.0` (float). `scale = 1.0`, `offset = 0.0`.
- `dst = round(2.0 * 1.0 + 0.0) = 2` (int8). Host verifies exact integer `== 2`.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
