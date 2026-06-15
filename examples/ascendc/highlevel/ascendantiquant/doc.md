# Ascend C · AscendAntiQuant (high-level, quantization)

- Category: high-level adv_api / quantization
- Covered API: `AscendAntiQuant` (per-tensor scalar)
- include: `lib/quantization/ascend_antiquant.h`
- Source: CANN 9.1.0 Ascend C API Reference / AscendAntiQuant

## Functionality
Anti-quantization. Per the header and impl the formula is `dst = scale * (src + offset)`
(the offset is **added first, then scaled** — note this differs from the common
`(src - offset) * scale` convention; here offset is added). The scalar overload takes
`offset` and `scale` as scalars of the **output** type.

## Measured signature
```cpp
template <typename InputDataType, typename OutputDataType, bool isTranspose>
void AscendAntiQuant(const LocalTensor<OutputDataType>& dst, const LocalTensor<InputDataType>& src,
                     const OutputDataType offset, const OutputDataType scale,
                     const LocalTensor<uint8_t>& sharedTmpBuffer,
                     const uint32_t k, const AntiQuantShapeInfo& shapeInfo = {});
```
- `src` here is `int8_t`; `dst`/`offset`/`scale` are `half`.
- With `isTranspose = false`, the whole `src.GetSize()` is processed; `k` only matters
  for the transpose path.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>` (over-allocated to 8 KB here).

## Minimal example design
- `ELEM = 256`, src all `2` (int8). `offset = 0.0`, `scale = 1.0`.
- `dst = 1.0 * (2 + 0.0) = 2.0` (half). IEEE half `2.0` is exactly `0x4000`, so the host
  compares the raw 16-bit pattern against `0x4000` (exact, no host half library needed).
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
