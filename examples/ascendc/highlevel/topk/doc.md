# Ascend C · TopK (high-level, host computes Tiling)

- Category: high-level adv_api / sorting
- APIs covered: `AscendC::TopK` + host `AscendC::TopKTilingFunc`
- include (kernel): `lib/sort/topk.h`; (host): `tiling/topk/topk_tiling.h`, `tiling/platform/platform_ascendc.h`
- Source: CANN 9.1.0 Ascend C API Reference / TopK

## Function
`TopK<T, isReuseSrc, isHasFinish, isInitIndex(template), mode>(dstV, dstIdx, srcV, srcIdx, finish, tmp, k, tiling, info, isLargest)`:
within each row of length `inner`, take the largest/smallest `k` values and their indices.

## host tiling mode (isomorphic to Matmul)
TopK requires an external `TopkTiling`, computed by the host and passed into the kernel via GM:
```cpp
// host
auto *platform = platform_ascendc::PlatformAscendCManager::GetInstance();
AscendC::tiling::TopkTiling tiling;
AscendC::TopKTilingFunc(*platform, INNER, OUTTER, K, sizeof(float),
                        /*isInitIndex*/true, AscendC::TopKMode::TOPK_NORMAL,
                        /*isLargest*/true, tiling);
// memcpy(&tiling) -> GM -> inside the kernel, copy back word by word into AscendC::tiling::TopkTiling as uint32
```
> Key: you must fill `AscendC::tiling::TopkTiling` (not `optiling::*`, whose memory layout differs and would corrupt it).

## Compilation notes
- Link `libtiling_api.a` + `libplatform.so` + `libgraph_base.so` (the last provides `ge::Shape`).
- include must add `aarch64-linux/asc/include/adv_api/sort` (otherwise `topk_utils_constants.h` is not found).

## Minimal example design
- Input `[OUTTER=1, INNER=32]`, `src[i]=i`, `idx[i]=i`, `K=4`, `isLargest=true`.
- Expect top4 values `[31,30,29,28]`, indices `[31,30,29,28]`. Single core, host verify errors=0 (instr≈561).
