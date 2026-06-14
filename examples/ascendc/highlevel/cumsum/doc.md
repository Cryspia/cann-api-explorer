# Ascend C · CumSum (high-level, prefix sum)

- Category: high-level adv_api / math
- Covered API: `CumSum`, `CumSumInfo`, `CumSumConfig`
- include: `lib/math/cumsum.h`
- Source: CANN 9.1.0 Ascend C API Reference / CumSum

## Functionality
`CumSum<T, config>(dst, lastRow, src, sharedTmpBuffer, CumSumInfo{outter, inner})`:
last-axis prefix sum over a `[outter, inner]` tensor, `dst[r][c] = sum(src[r][0..c])`.

- The default `CumSumConfig` is `{isLastAxis=true, isReuseSource=false, outputLastRow=true}`,
  so `CumSum` also writes the prefix sum of the final row into `lastRow` (length `inner`).
- Supported dtypes: `half`, `float`.
- `sharedTmpBuffer` is a `LocalTensor<uint8_t>`; the last-axis path needs at least
  `16 * inner * sizeof(T) * 2` bytes (here ~2KB; the unit allocates 16KB).
- Availability: the device path is guarded by `__NPU_ARCH__` in `{2201,2002,3510,5102,3003,3113}`.
  Ascend950PR (the simulator's `dav_3510`) maps to 3510.

## Minimal example design
- Shape `[16,16]`: `outter` and `inner` are multiples of `NCHW_CONV_ADDR_LIST_SIZE`(16), which the
  device-side transpose path requires; `inner=16` floats = 64 bytes (32-byte aligned).
- `src` is all ones. Last-axis prefix sum makes each output row `[1,2,3,...,16]`,
  and `lastRow` (prefix sum of the final input row) is also `[1,2,...,16]`.
- Single-core execution, host element-wise verification of both `dst` and `lastRow` (tol 1e-3).
  See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
