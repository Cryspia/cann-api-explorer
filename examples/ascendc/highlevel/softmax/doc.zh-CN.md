# Ascend C · SoftMax（高阶，带 Tiling）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`SoftMax`、`SoftMaxTilingFunc`（device 端构造 tiling）
- include：`lib/activation/softmax.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SoftMax

## 功能
`SoftMax(dst, src, sharedTmpBuffer, tiling, shapeInfo)`：对 `[M,K]` 沿最后一维 K 做 softmax，
`y = exp(x-max)/sum(exp(x-max))`。

## 关键：kernel 内构造 Tiling（避开 host tiling 框架）
带 `*Tiling` 的高阶算子通常要 host 侧算 tiling 再传入。本例改用 **device 端可调用**的
`AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)` 在 kernel 内直接构造：
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftMax<float>(zLocal, xLocal, tmp /*uint8 sharedTmpBuffer*/, tiling, info);
```
- `SoftMaxTiling` 在命名空间 `AscendC::tiling`；`SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftMax` 在 `AscendC`。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`，本例给 16KB VECCALC 空间。

## 最简 example 设计
- 形状 `[M,K]=[8,64]`，src 全填 `1.0`。沿 K 做 softmax → 每元素 `1/64 = 0.015625`。
- 单核执行，host 逐元素校验（tol 1e-3）。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
