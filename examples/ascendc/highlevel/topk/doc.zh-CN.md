# Ascend C · TopK（高阶，host 算 Tiling）

- 分类：高阶 adv_api / 排序
- 覆盖 API：`AscendC::TopK` + host `AscendC::TopKTilingFunc`
- include（kernel）：`lib/sort/topk.h`；（host）：`tiling/topk/topk_tiling.h`、`tiling/platform/platform_ascendc.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / TopK

## 功能
`TopK<T, isReuseSrc, isHasFinish, isInitIndex(模板), mode>(dstV, dstIdx, srcV, srcIdx, finish, tmp, k, tiling, info, isLargest)`：
在每个 `inner` 长度的行内取最大/最小的 `k` 个值及其下标。

## host tiling 模式（与 Matmul 同构）
TopK 需要外部 `TopkTiling`，由 host 计算后经 GM 传入 kernel：
```cpp
// host
auto *platform = platform_ascendc::PlatformAscendCManager::GetInstance();
AscendC::tiling::TopkTiling tiling;
AscendC::TopKTilingFunc(*platform, INNER, OUTTER, K, sizeof(float),
                        /*isInitIndex*/true, AscendC::TopKMode::TOPK_NORMAL,
                        /*isLargest*/true, tiling);
// memcpy(&tiling) → GM → kernel 内 uint32 逐字拷回 AscendC::tiling::TopkTiling
```
> 关键：必须填 `AscendC::tiling::TopkTiling`（非 `optiling::*`，内存布局不同会损坏）。

## 编译要点
- 链接 `libtiling_api.a` + `libplatform.so` + `libgraph_base.so`（后者提供 `ge::Shape`）。
- include 需加 `aarch64-linux/asc/include/adv_api/sort`（否则 `topk_utils_constants.h` 找不到）。

## 最简 example 设计
- 输入 `[OUTTER=1, INNER=32]`，`src[i]=i`，`idx[i]=i`，`K=4`，`isLargest=true`。
- 期望 top4 值 `[31,30,29,28]`，下标 `[31,30,29,28]`。单核，host 校验 errors=0（instr≈561）。
