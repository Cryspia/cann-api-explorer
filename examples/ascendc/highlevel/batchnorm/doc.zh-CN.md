# Ascend C · BatchNorm（高阶，3 字段手填 Tiling）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`AscendC::BatchNorm`
- include：`lib/normalization/batchnorm.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / BatchNorm

## 功能
`BatchNorm<T,isReuseSrc,isBasicBlock>(output, outputMean, outputVariance, x, gamma, beta, tmp, epsilon, tiling)`：
`output = gamma * (x - mean) * rsqrt(var + eps) + beta`，其中 `mean = sum_b(x) / B`，**沿 first dim B** 归一化。
内存布局 `[B, F]` row-major，即 `srcLocal[b*F + f]`；mean/var/gamma/beta 都是 per-feature `[F]`。

## 关键：3510 只读 3 字段，含系数 firstDimValueBack
`BatchNormTiling` 有 20 字段，3510 impl 只引用 `originalBLength / meanVarSize / firstDimValueBack`：
- `originalBLength = B`（batch 长度，归约维）；`meanVarSize = F`（feature 长度）。
- `firstDimValueBack = 1/B`：mean 系数，`ComputeOutputMean` 内对每个 b 累加时 `Reg::Muls(srcReg, firstDimValueBack)`。**直接 1/B 即可，无需反推**。
```cpp
AscendC::tiling::BatchNormTiling t;
t.originalBLength = B;            // 4
t.meanVarSize = F;               // 8
t.firstDimValueBack = 1.0f / B;  // 0.25
AscendC::BatchNorm<float, false, false>(zL, meanL, varL, xL, gammaL, betaL, tmp, /*eps*/1e-5f, t);
```
> 与 GroupNorm 的 `factor=1/(D*HW)` 同思路：归一化维的元素个数倒数即 mean 系数。

## 最简 example 设计
- `[B=4,F=8]`，`x` 全 `5` → 每 feature 4 个 batch 值相等 → `var=0`。
- `gamma=1, beta=2` → `output = 0*gamma + beta = 2`，`mean=5`。单核，host 校验 errors=0（instr≈863）。
