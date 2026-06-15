# Ascend C · SimpleSoftMax（高阶，带 Tiling）

- 分类：高阶 adv_api / 激活（softmax）
- 覆盖 API：`SimpleSoftMax`、`SoftMaxTilingFunc`（device 端构造 tiling）
- include：`lib/activation/simplesoftmax.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SimpleSoftMax

## 功能
`SimpleSoftMax(dst, inSum, inMax, src, sharedTmpBuffer, tiling, shapeInfo)`：对 `[M,K]` 沿最后一维 K
做简化 softmax。它接收已经预先算好的每行 max 与 sum，只执行最后的归一化步骤：
```
y = exp(x - inMax) / inSum
```
已对照 3510 实现确认（先 `Sub`，再 `Exp`，再对 inSum 做 `Div`）。

## 关键：kernel 内构造 Tiling（避开 host tiling 框架）
本例复用 device 端可调用的 `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
在 kernel 内直接构造 `SoftMaxTiling`：
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SimpleSoftMax<float>(zLocal, sumLocal, maxLocal, xLocal, tmp, tiling, info);
```
- `SoftMaxTiling` 在命名空间 `AscendC::tiling`；`SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SimpleSoftMax` 在 `AscendC`。
- `inMax` / `inSum` 采用 AscendC 归约 block 布局：每行标量位于其 32B（8 个 float）block 的第 0 列。
- 支持 dtype：half 和 float（以及 half 源 / float 统计量组合）。

## 最简示例设计
- 形状 `[M,K]=[8,64]`。
- `x` 全 `0.0`；`inMax` 每行 `0.0`；`inSum` 每行 `64.0`。
- 则 `y = exp(0 - 0) / 64 = 1/64 = 0.015625`，各元素相同。
- 期望输出：全部为 `0.015625`。单核执行，host 端逐元素校验（容差 1e-4）。
  见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
