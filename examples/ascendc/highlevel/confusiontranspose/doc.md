# Ascend C · ConfusionTranspose (high-level)

- Category: high-level adv_api / transpose
- Covered API: `AscendC::Transpose` (confusion-transpose family, with `ConfusionTransposeTiling`)
- include: `adv_api/transpose/confusion_transpose.h`
- Source: CANN 9.1.0 Ascend C API Reference / ConfusionTranspose

## Confirmed signature (`__NPU_ARCH__==3510`)
```cpp
template <typename T>
void Transpose(const LocalTensor<T>& dst, const LocalTensor<T>& src,
               const LocalTensor<uint8_t>& sharedTmpBuffer,
               TransposeType transposeType, ConfusionTransposeTiling& tiling);
```
- The function is named `Transpose` (not `ConfusionTranspose`). `TransposeType` is an enum covering the
  attention reshape+transpose scenes (`TRANSPOSE_NZ2ND_0213`, `TRANSPOSE_NZ2NZ_012_*`, ...).
- `ConfusionTransposeTiling` is a generic struct of 18 `uint32_t` fields (`param0..param17`); each scene
  reinterpret_casts it to a scene-specific tiling.
- On 3510 the simplest, format-agnostic mode is `TRANSPOSE_ND2ND_021`: it transposes the last two axes of
  a `[dim0, dim1, dim2]` ND tensor -> `[dim0, dim2, dim1]`. Its tiling is `{dim0, dim1, dim2}`, i.e.
  `param0=dim0, param1=dim1, param2=dim2`. With `dim0=1` this degenerates to a plain `[H,W] -> [W,H]`.

## Minimal verifiable design
- `H=W=16`, `T=half`, `tiling={1,16,16}`, type `TRANSPOSE_ND2ND_021`.
- `src[h][w] = h*16 + w` -> expect `dst[i][j] = src[j][i] = j*16 + i`.
- Host verifies the full transposed 16x16 matrix. Measured: `dst[0][1]=16, dst[1][0]=1, dst[15][15]=255`, errors=0.

Note: `ConfusionTransposeTiling` lives in `AscendC::tiling` (also exposed at global scope via a `using`);
the `kernel.cpp` uses `AscendC::tiling::ConfusionTransposeTiling`.

See `kernel.cpp` / `main.cpp`, result in `RESULT.md`.
