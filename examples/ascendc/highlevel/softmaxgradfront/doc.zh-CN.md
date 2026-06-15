# Ascend C · SoftmaxGradFront（高阶，带 Tiling）

- 分类：高阶 adv_api / 激活（softmax 反向的前半段）
- 覆盖 API：`SoftmaxGradFront`、`SoftMaxTilingFunc`（device 端构造 tiling）
- include：`lib/activation/softmaxgrad.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SoftmaxGradFront

## 功能
`SoftmaxGradFront(dst, grad, x, sharedTmpBuffer, tiling, shapeInfo)`：softmax 反向的「前半段」，
对 `[M,K]` 沿最后一维 K，其中 `x` 为前向 softmax 输出。它只产生每行的归约：
```
y = rowsum(grad * x)
```
（区别于 `SoftmaxGrad` 的 `isFront=false`，后者返回完整的 `grad*x - sum*x`）。3510 实现把每行一个标量
写入该行 32B block 的第 0 列，因此输出是归约形状（`M` 行，值在每个 8 个 float block 的第 0 列）。

## 关键：kernel 内构造 Tiling（避开 host tiling 框架）
本例复用 device 端可调用的 `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
在 kernel 内直接构造 `SoftMaxTiling`：
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftmaxGradFront<float>(zLocal, gradLocal, xLocal, tmp, tiling, info);
```
- `SoftMaxTiling` 在 `AscendC::tiling`；`SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftmaxGradFront` 在 `AscendC`。
- `sharedTmpBuffer` 是 `LocalTensor<uint8_t>`；本例给它 16KB 的 VECCALC 空间。
- 支持 dtype：half 和 float。

## 最简示例设计
- 形状 `[M,K]=[8,64]`。
- `x` 为均匀前向 softmax 输出 `1/K = 0.015625`（每行和为 1）。
- `grad` 全为常数 `c = 2.0`。
- 则每行 `y = rowsum(grad*x) = K*(c/K) = c = 2.0`。
- 期望输出：每行标量 `= 2.0`（读 `y[row*8]`）。单核执行，host 端逐行校验（容差 1e-3）。
  见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
