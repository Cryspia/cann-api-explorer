# Ascend C · AscendDequant (high-level, quantization)

- Category: high-level adv_api / quantization
- Covered API: `AscendDequant`
- include: `lib/quantization/ascend_dequant.h`
- Source: CANN 9.1.0 Ascend C API Reference / AscendDequant

## Functionality
Dequantization: `dst(i) = src(i) * deqScale(i % deqScale.GetSize())`.
The source is **always `int32`**. `deqScale` is a per-channel `LocalTensor` that is
recycled every `deqScale.GetSize()` elements (`srcTensor.GetSize() % deqScale.GetSize() == 0`
is required). A scalar-`deqScale` form (with `DequantParams`) also exists.

## Measured signature
```cpp
template <typename dstT, typename scaleT, DeQuantMode mode = DeQuantMode::DEQUANT_WITH_SINGLE_ROW>
void AscendDequant(const LocalTensor<dstT>& dst, const LocalTensor<int32_t>& src,
                   const LocalTensor<scaleT>& deqScale,
                   const LocalTensor<uint8_t>& sharedTmpBuffer, const uint32_t calCount);
```
- `src` is fixed `int32_t`; `dstT` here is `float`, `scaleT` here is `float`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>` (over-allocated to 8 KB here).

## Minimal example design
- `ELEM = 256`, src all `4` (int32). `deqScale` is a length-`ELEM` tensor filled with `0.5`.
- `dst = 4 * 0.5 = 2.0` (float). Host verifies `== 2.0` with tolerance `1e-4`.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
