# Ascend C · TransData (high-level)

- Category: high-level adv_api / transpose (format conversion)
- Covered API: `AscendC::TransData` (ND <-> fractal 5D format conversion)
- include: `adv_api/transpose/transdata.h`
- Source: CANN 9.1.0 Ascend C API Reference / TransData

## Confirmed signature (`__NPU_ARCH__==3510`)
```cpp
template <const TransDataConfig& config, typename T, typename U, typename S>
void TransData(const LocalTensor<T>& dst, const LocalTensor<T>& src,
               const LocalTensor<uint8_t>& sharedTmpBuffer,
               const TransDataParams<U, S>& params);   // params = {srcLayout, dstLayout}
```
- `config` is a compile-time `TransDataConfig{srcFormat, dstFormat}`. On 3510 the supported pairs are
  `NCDHW <-> NDC1HWC0` and `NCDHW <-> FRACTAL_Z_3D` (5D format conversion).
- `T`: `half` / `bfloat16` / `uint16` / `int16`.
- `params.srcLayout` / `params.dstLayout` are CuTe-style `Layout` objects built with
  `MakeLayout(MakeShape(...), MakeStride(...))`. NCDHW shape has 5 dims, NDC1HWC0 has 6, FRACTAL_Z_3D has 7
  (enforced by `static_assert`). The 3510 reorder only reads the NCDHW-side `(n,c,d,h,w)`.
- The conversion is built on `TransDataTo5HD` 16x16 block transposes; it requires a `uint8_t`
  `sharedTmpBuffer` (here 16KB).

## Note on the host compile pass
`transdata.h` gates `TransDataConfig` and `TransData` behind `__NPU_ARCH__`, which is **undefined** on the
host-only kernel compile pass. The device code (config + `TransData` calls) is therefore wrapped in
`#if defined(__NPU_ARCH__)` so the host pass still produces the launch stub.

## Minimal verifiable design (round trip)
Rather than hand-decoding the fractal byte layout, this unit verifies an identity **round trip**:
`NCDHW -> NDC1HWC0 -> NCDHW`. Shape NCDHW `[1,16,1,4,4]` (`C=16=c0` so no channel padding; `H*W=16` so no
spatial padding), `T=half`, `src[i] = i+1`. The round trip must reproduce all 256 elements.
Measured: `d[0]=1, d[128]=129, d[255]=256`, errors=0.

See `kernel.cpp` / `main.cpp`, result in `RESULT.md`.
