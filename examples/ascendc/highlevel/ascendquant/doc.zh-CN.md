# Ascend C · AscendQuant（高阶，量化）

- 分类：高阶 adv_api / 量化
- 覆盖 API：`AscendQuant`（per-tensor）
- include：`lib/quantization/ascend_quant.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / AscendQuant

## 功能
per-tensor 量化：`dst(int8) = round(src * scale + offset)`。
`scale`、`offset` 为 **标量 float**（整张量共用）。另有 per-channel 形式（scale/offset 为
`LocalTensor`），本例未用。

## 实测签名
```cpp
template <typename T, bool isReuseSource = false, const AscendQuantConfig& config = ASCEND_QUANT_DEFAULT_CFG>
void AscendQuant(const LocalTensor<int8_t>& dst, const LocalTensor<T>& src,
                 const LocalTensor<uint8_t>& sharedTmpBuffer,
                 const float scale, const float offset, const uint32_t calCount);
```
- `T` 支持 `half` / `float`；输出固定 `int8_t`。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`（大小由 tiling API 给出，本例多给 8 KB）。

## 最简 example 设计
- `ELEM = 256`，src 全填 `2.0`（float）。`scale = 1.0`，`offset = 0.0`。
- `dst = round(2.0 * 1.0 + 0.0) = 2`（int8）。host 按整型精确校验 `== 2`。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
