# Ascend C · CumSum（高阶，前缀和）

- 分类：高阶 adv_api / math
- 覆盖 API：`CumSum`、`CumSumInfo`、`CumSumConfig`
- include：`lib/math/cumsum.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / CumSum

## 功能
`CumSum<T, config>(dst, lastRow, src, sharedTmpBuffer, CumSumInfo{outter, inner})`：
对 `[outter, inner]` 张量沿最后一维做前缀和，`dst[r][c] = sum(src[r][0..c])`。

- 默认 `CumSumConfig` 为 `{isLastAxis=true, isReuseSource=false, outputLastRow=true}`，
  因此 `CumSum` 还会把最后一行的前缀和写入 `lastRow`（长度 `inner`）。
- 支持 dtype：`half`、`float`。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`；最后一维路径至少需要
  `16 * inner * sizeof(T) * 2` 字节（此处约 2KB；本单元给 16KB）。
- 可用性：device 实现以 `__NPU_ARCH__ ∈ {2201,2002,3510,5102,3003,3113}` 为条件。
  Ascend950PR（仿真器的 `dav_3510`）对应 3510。

## 最简 example 设计
- 形状 `[16,16]`：`outter` 与 `inner` 均为 `NCHW_CONV_ADDR_LIST_SIZE`(16) 的倍数，
  这是 device 端转置路径所需；`inner=16` 个 float = 64 字节（32 字节对齐）。
- `src` 全 1。沿最后一维前缀和后每行输出 `[1,2,3,...,16]`，
  `lastRow`（最后一行的前缀和）同样为 `[1,2,...,16]`。
- 单核执行，host 端对 `dst` 与 `lastRow` 逐元素校验（tol 1e-3）。
  见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
