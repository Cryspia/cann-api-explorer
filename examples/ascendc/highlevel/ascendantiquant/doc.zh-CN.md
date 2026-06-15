# Ascend C · AscendAntiQuant（高阶，量化）

- 分类：高阶 adv_api / 量化
- 覆盖 API：`AscendAntiQuant`（per-tensor 标量）
- include：`lib/quantization/ascend_antiquant.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / AscendAntiQuant

## 功能
反量化（anti-quant）。按头文件与 impl，公式为 `dst = scale * (src + offset)`
（offset **先加再乘** scale —— 注意这与常见的 `(src - offset) * scale` 约定不同，此处是相加）。
标量重载中 `offset`、`scale` 为 **输出类型** 的标量。

## 实测签名
```cpp
template <typename InputDataType, typename OutputDataType, bool isTranspose>
void AscendAntiQuant(const LocalTensor<OutputDataType>& dst, const LocalTensor<InputDataType>& src,
                     const OutputDataType offset, const OutputDataType scale,
                     const LocalTensor<uint8_t>& sharedTmpBuffer,
                     const uint32_t k, const AntiQuantShapeInfo& shapeInfo = {});
```
- 本例 `src` 为 `int8_t`；`dst`/`offset`/`scale` 为 `half`。
- `isTranspose = false` 时处理整段 `src.GetSize()`；`k` 仅在 transpose 路径有用。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`（本例多给 8 KB）。

## 最简 example 设计
- `ELEM = 256`，src 全填 `2`（int8）。`offset = 0.0`，`scale = 1.0`。
- `dst = 1.0 * (2 + 0.0) = 2.0`（half）。IEEE half 的 `2.0` 恰为 `0x4000`，host 直接按
  16-bit 位模式与 `0x4000` 精确比较（无需 host half 库）。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
