# Ascend C · ConfusionTranspose（高阶）

- 分类：高阶 adv_api / transpose
- 覆盖 API：`AscendC::Transpose`（confusion-transpose 家族，带 `ConfusionTransposeTiling`）
- include：`adv_api/transpose/confusion_transpose.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / ConfusionTranspose

## 实测签名（`__NPU_ARCH__==3510`）
```cpp
template <typename T>
void Transpose(const LocalTensor<T>& dst, const LocalTensor<T>& src,
               const LocalTensor<uint8_t>& sharedTmpBuffer,
               TransposeType transposeType, ConfusionTransposeTiling& tiling);
```
- 函数名是 `Transpose`（不是 `ConfusionTranspose`）。`TransposeType` 枚举覆盖 attention 的
  reshape+transpose 场景（`TRANSPOSE_NZ2ND_0213`、`TRANSPOSE_NZ2NZ_012_*` 等）。
- `ConfusionTransposeTiling` 是含 18 个 `uint32_t`（`param0..param17`）的通用结构体；各场景把它
  reinterpret_cast 成场景专用 tiling。
- 3510 上最简单、与格式无关的模式是 `TRANSPOSE_ND2ND_021`：对 `[dim0, dim1, dim2]` ND 张量转置最后两轴
  -> `[dim0, dim2, dim1]`。其 tiling 为 `{dim0, dim1, dim2}`，即 `param0=dim0, param1=dim1, param2=dim2`。
  `dim0=1` 时退化为普通 `[H,W] -> [W,H]`。

## 最小可校验设计
- `H=W=16`，`T=half`，`tiling={1,16,16}`，类型 `TRANSPOSE_ND2ND_021`。
- `src[h][w] = h*16 + w` -> 期望 `dst[i][j] = src[j][i] = j*16 + i`。
- host 校验完整转置的 16x16 矩阵。实测：`dst[0][1]=16, dst[1][0]=1, dst[15][15]=255`，errors=0。

注意：`ConfusionTransposeTiling` 在 `AscendC::tiling` 命名空间（也通过 `using` 暴露到全局）；
`kernel.cpp` 用 `AscendC::tiling::ConfusionTransposeTiling`。

详见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
