# Ascend C · SoftmaxFlashV3（高阶，带 Tiling）

- 分类：高阶 adv_api / 激活（FlashAttention-2 在线 softmax，v3 变体）
- 覆盖 API：`SoftmaxFlashV3`、`SoftMaxTilingFunc`（device 端构造 tiling）、`SoftMaxParams`
- include：`lib/activation/softmaxflashv3.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SoftmaxFlashV3

## 功能
`SoftmaxFlashV3<T,U,isUpdate>(dst, mean, sum, max, src, expMax, inMean, inSum, inMax, sharedTmpBuffer, tiling, params)`：
FlashAttention-2 内部使用的 v3 在线 softmax。相比 FlashV2 增加了显式的每行 mean 步骤与一个 shift 项。
首块（`isUpdate = false`）：
```
rowMeanGlobal = rowsum(x) / K           （每行）
mean          = rowMeanGlobal
x'            = x - meanTmp * alpha/(1-alpha)
max           = rowmax(x')
y             = exp(x' - max)           （未归一，不除以 sum）
sum           = rowsum(y)
```
API 固定 dtype：`T`（src / dst / expMax）= `half`，`U`（mean / sum / max）= `float`（impl 中有 `static_assert` 强制）。

## 关键：device 端 tiling + FlashV3 参数
tiling 与普通 softmax 相同（`SoftMaxTiling`），因此在 kernel 内用 device 端可调用的
`AscendC::SoftMaxTilingFunc` 构造。FlashV3 专有参数放在 `AscendC::SoftMaxParams`：
```cpp
AscendC::SoftMaxParams params;
params.srcM = M; params.srcK = K; params.oriSrcM = M; params.oriSrcK = K;
params.splitMeanCnt = 1;   // K=64 -> kRepeatTime=1，单 split 方案才成立
AscendC::SoftmaxFlashV3<half, float, false>(z, mean, sum, max, x, expMax,
                                            inMean, inSum, inMax, tmp, tiling, params);
```
- `SoftMaxTiling` 在 `AscendC::tiling`；`SoftMaxParams`/`SoftMaxTilingFunc`/`SoftmaxFlashV3` 在 `AscendC`。
- `splitMeanCnt` 默认为 8；K=64 时向量 repeat 覆盖整行（kRepeatTime=1），故必须设为 1，
  否则 `remainRepeatTime = kRepeatTime - splitMeanCnt` 会下溢。
- 归约输出（mean / sum / max）采用 AscendC block 布局：值在每个 8 个 float block 的第 0 列。

## 最简示例设计
- 形状 `[M,K]=[8,64]`，`isUpdate = false`（首块），故 inMean / inSum / inMax / expMax 不参与。
- `x` 全 `0.0`（half）。每行统计量都坍缩为 0，shift 后的输入 `x' = 0`。
- 因此 `mean = 0`、`max = 0`、`y = exp(0) = 1.0`（未归一）、`sum = K = 64`。
- 期望：dst 全 `1.0`；每行 `mean = 0`、`max = 0`、`sum = 64`。单核执行，host 端 half 编解码校验
  （half 值容差 1e-2）。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
