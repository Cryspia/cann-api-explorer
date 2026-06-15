# Ascend C · Quantize（高阶，quantization）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / quantization
- 覆盖 API：`Quantize`（通用量化 API）
- include：`lib/quantization/quantize.h`
- 来源：CANN 9.1.0 Ascend C API Reference / Quantize

## 功能
`Quantize` 是通用量化 API：`dst = round(src * scale + offset)`。

它**区别于 `AscendQuant`**（已由 `ascendquant` 单元覆盖）。虽然 per-tensor 算术相同，但 API 形态不同：
- `Quantize` 带 `QuantizeConfig{policy, hasOffset, roundMode, kDim}` 非类型模板参数，以及
  `QuantizeParams{m, n, groupSize}` 结构。
- 支持四种策略 —— `PER_TENSOR`、`PER_CHANNEL`、`PER_TOKEN`、`PER_GROUP` —— 以及通用 dst 类型，
  scale/offset 可为标量或 `LocalTensor`。
- `AscendQuant` 是较简单的 per-tensor/per-channel 形式，scale/offset 固定为标量（或 tensor），
  dst 固定为 `int8`。

## 实测签名
```cpp
template <const QuantizeConfig& config, typename DstT, typename SrcT, typename ScaleT, typename OffsetT>
void Quantize(const LocalTensor<DstT>& dstTensor, const LocalTensor<SrcT>& srcTensor,
              const ScaleT& scale, const OffsetT& offset, const QuantizeParams& params);
// 另有 sharedTmpBuffer 重载。

struct QuantizeConfig { QuantizePolicy policy; bool hasOffset; RoundMode roundMode = CAST_RINT; int32_t kDim = 1; };
struct QuantizeParams { uint32_t m; uint32_t n; uint32_t groupSize = 0; };
enum class QuantizePolicy { PER_TENSOR, PER_CHANNEL, PER_TOKEN, PER_GROUP };
```
- `PER_TENSOR` 下 `ScaleT`/`OffsetT` 必须为标量（此处 `float`）；无需 tmp buffer。
- 该 API 仅在 `__NPU_ARCH__ == 3510 / 5102` 上启用。

## 最简示例设计
- `PER_TENSOR`、`m = 1`、`n = 256`、`hasOffset = true`、`CAST_RINT`。
- src 全 `2.0`（float），`scale = 2.0`，`offset = 1.0`。
- `dst = round(2*2 + 1) = 5`（int8）。host 精确校验整数 `== 5`。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
