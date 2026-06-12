# Ascend C · Sort（高阶，免 Tiling）

- 分类：高阶 adv_api / 排序
- 覆盖 API：`AscendC::Sort`
- include：`lib/sort/sort.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / Sort

## 功能
`Sort<T, U>(dstValue, dstIndex, srcValue, srcIndex, sharedTmpBuffer, calCount)`：
对前 `calCount` 个元素按 value 排序，并同步搬运每个元素携带的 index。32 个元素为一个排序 region。

## 关键发现：本实现为**升序**
经无卡仿真实测（逆序输入 `val=[31..0], idx=[0..31]`）：
```
dst[ 0] val=0  idx=31     # 最小值排在最前
dst[31] val=31 idx=0      # 最大值排在最后
```
即 `AscendC::Sort` 输出按 value **升序**，`dstIndex[i]` 为该值在原数组中的位置。
> 踩坑记录：最初按“硬件 Sort 默认降序”的惯例写期望，且用升序输入 `[0..31]`，
> 升序排序后输出不变 → 误判为“没排序/identity 输出”。改用**逆序输入**才区分出真实方向。

## 最简 example 设计
- `N=32` float，输入 `val[i]=31-i`（逆序），`idx[i]=i`。
- 期望：`dst[i]=i`（升序），`dstIdx[i]=31-i`（值 i 的原始下标）。
- 免 tiling：`Sort` 的 count 模式不需要外部 tiling 结构，`tmp` 给 8192B 足够。
- 单核，host 校验 errors=0（见 `RESULT.md`，instr≈1774）。

## 对比 TopK
TopK 需 host `TopKTilingFunc` 算 `TopkTiling` 经 GM 传入；Sort 的 count 重载完全免 tiling，更轻量。
两者都在 `lib/sort/` 下，编译需加 include 路径 `aarch64-linux/asc/include/adv_api/sort`。
