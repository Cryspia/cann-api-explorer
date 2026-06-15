# Ascend C · AscendDequant（高阶，量化）

- 分类：高阶 adv_api / 量化
- 覆盖 API：`AscendDequant`
- include：`lib/quantization/ascend_dequant.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / AscendDequant

## 功能
反量化：`dst(i) = src(i) * deqScale(i % deqScale.GetSize())`。
源张量 **固定为 `int32`**。`deqScale` 为 per-channel `LocalTensor`，每
`deqScale.GetSize()` 个元素循环一次（要求 `srcTensor.GetSize() % deqScale.GetSize() == 0`）。
另有标量 `deqScale` 形式（配合 `DequantParams`）。

## 实测签名
```cpp
template <typename dstT, typename scaleT, DeQuantMode mode = DeQuantMode::DEQUANT_WITH_SINGLE_ROW>
void AscendDequant(const LocalTensor<dstT>& dst, const LocalTensor<int32_t>& src,
                   const LocalTensor<scaleT>& deqScale,
                   const LocalTensor<uint8_t>& sharedTmpBuffer, const uint32_t calCount);
```
- `src` 固定 `int32_t`；本例 `dstT` 为 `float`，`scaleT` 为 `float`。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`（本例多给 8 KB）。

## 最简 example 设计
- `ELEM = 256`，src 全填 `4`（int32）。`deqScale` 为长度 `ELEM`、全填 `0.5` 的张量。
- `dst = 4 * 0.5 = 2.0`（float）。host 以容差 `1e-4` 校验 `== 2.0`。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
