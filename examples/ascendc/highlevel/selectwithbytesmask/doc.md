# Ascend C · SelectWithBytesMask (high-level)

- Category: high-level adv_api / select
- Covered API: `AscendC::Select` (tensor-scalar overload, byte-mask)
- include: `adv_api/select/selectwithbytesmask.h`
- Source: CANN 9.1.0 Ascend C API Reference / SelectWithBytesMask

## Confirmed signature (`__NPU_ARCH__==3510`)
```cpp
template <typename T, typename U, bool isReuseMask = true>
void Select(const LocalTensor<T>& dst, const LocalTensor<T>& src0, T src1,
            const LocalTensor<U>& mask, const LocalTensor<uint8_t>& sharedTmpBuffer,
            const SelectWithBytesMaskShapeInfo& info);
```
- `T`: `half` / `float`. `U` (mask): `bool/uint8/int8/uint16/int16/uint32/int32`.
- `SelectWithBytesMaskShapeInfo{firstAxis, srcLastAxis, maskLastAxis}`. The tensor is viewed as
  `[firstAxis, lastAxis]`. Constraints: `srcLastAxis*sizeof(T)` 32B aligned; `maskLastAxis*sizeof(U)`
  32B aligned and a multiple of 16; `maskLastAxis >= srcLastAxis`.
- Semantics (this tensor-scalar overload): mask byte `== 0` -> take **src0 tensor**;
  mask byte `!= 0` -> take **src1 scalar**. (A second overload swaps which side is the scalar.)

## Minimal verifiable design
- Shape `[16,16]`, `T=half`, `U=uint8`. `src0[i] = i`, `src1` scalar `= 99`.
- Mask per row: first 8 columns `0` (keep src0 = index), last 8 columns `1` (take scalar 99).
- Host rebuilds the expected matrix element-by-element. Measured: `d[0]=0, d[8]=99, d[15]=99`, errors=0.

See `kernel.cpp` / `main.cpp`, result in `RESULT.md`.
