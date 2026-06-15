# Ascend C · SoftmaxGrad（高阶，带 Tiling）

- 分类：高阶 adv_api / 归一化（softmax 反向）
- 覆盖 API：`SoftmaxGrad`、`SoftMaxTilingFunc`（device 端构造 tiling）
- include：`lib/activation/softmaxgrad.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SoftmaxGrad

## 功能
`SoftmaxGrad(dst, grad, x, sharedTmpBuffer, tiling, isFront, shapeInfo)`：对 `[M,K]` 沿最后一维 K 做
softmax 反向，其中 `x` 为前向 softmax 的输出。当 `isFront = false`：
```
sum = rowsum(grad * x)
y   = grad * x - sum * x
```
当 `isFront = true` 时改为返回 `y = rowsum(grad * x)`。

## 关键：kernel 内构造 Tiling（避开 host tiling 框架）
本例复用 device 端可调用的 `AscendC::SoftMaxTilingFunc(workLocalSize, shapeInfo, tiling, dtSize1, dtSize2)`
在 kernel 内直接构造 `SoftMaxTiling`：
```cpp
AscendC::SoftMaxShapeInfo info{M, K, M, K};
AscendC::tiling::SoftMaxTiling tiling;
AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling, sizeof(float), sizeof(float));
AscendC::SoftmaxGrad<float>(zLocal, gradLocal, xLocal, tmp /*uint8 sharedTmpBuffer*/, tiling, false, info);
```
- `SoftMaxTiling` 在命名空间 `AscendC::tiling`；`SoftMaxShapeInfo`/`SoftMaxTilingFunc`/`SoftmaxGrad` 在 `AscendC`。
- `sharedTmpBuffer` 为 `LocalTensor<uint8_t>`，本例给 16KB VECCALC 空间。
- 支持数据类型：half 与 float。

## 最简 example 设计
- 形状 `[M,K]=[8,64]`。
- `x` = 均匀的前向 softmax 输出 `1/K = 0.015625`（每行和为 1）。
- `grad` = 常数 `c = 2.0`。
- 则 `sum = rowsum(grad*x) = K*(c/K) = c = 2.0`，`y = grad*x - sum*x = c/K - c/K = 0`。
- 期望输出：所有元素恰为 `0.0`。单核执行，host 逐元素校验（tol 1e-3）。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
