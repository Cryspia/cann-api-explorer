# Ascend C · Quantize (high-level, quantization)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / quantization
- Covered API: `Quantize` (the general quantization API)
- include: `lib/quantization/quantize.h`
- Source: CANN 9.1.0 Ascend C API Reference / Quantize

## Functionality
`Quantize` is the general quantization API: `dst = round(src * scale + offset)`.

It is **distinct from `AscendQuant`** (covered by the `ascendquant` unit). Although the
per-tensor arithmetic is the same, the API surface differs:
- `Quantize` takes a `QuantizeConfig{policy, hasOffset, roundMode, kDim}` non-type template
  parameter and a `QuantizeParams{m, n, groupSize}` struct.
- It supports four policies — `PER_TENSOR`, `PER_CHANNEL`, `PER_TOKEN`, `PER_GROUP` — and a
  generic dst type, with scale/offset that can be scalars or `LocalTensor`s.
- `AscendQuant` is a simpler per-tensor/per-channel form with fixed scalar (or tensor)
  scale/offset and a fixed `int8` dst.

## Measured signature
```cpp
template <const QuantizeConfig& config, typename DstT, typename SrcT, typename ScaleT, typename OffsetT>
void Quantize(const LocalTensor<DstT>& dstTensor, const LocalTensor<SrcT>& srcTensor,
              const ScaleT& scale, const OffsetT& offset, const QuantizeParams& params);
// also a sharedTmpBuffer overload.

struct QuantizeConfig { QuantizePolicy policy; bool hasOffset; RoundMode roundMode = CAST_RINT; int32_t kDim = 1; };
struct QuantizeParams { uint32_t m; uint32_t n; uint32_t groupSize = 0; };
enum class QuantizePolicy { PER_TENSOR, PER_CHANNEL, PER_TOKEN, PER_GROUP };
```
- For `PER_TENSOR`, `ScaleT`/`OffsetT` must be scalars (here `float`); no tmp buffer needed.
- This API is enabled only on `__NPU_ARCH__ == 3510 / 5102`.

## Minimal example design
- `PER_TENSOR`, `m = 1`, `n = 256`, `hasOffset = true`, `CAST_RINT`.
- src all `2.0` (float), `scale = 2.0`, `offset = 1.0`.
- `dst = round(2*2 + 1) = 5` (int8). Host verifies exact integer `== 5`.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
