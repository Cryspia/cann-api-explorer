# Ascend C · GroupNorm（高阶，带 Tiling）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`GroupNorm`
- include：`lib/normalization/groupnorm.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / GroupNorm

## 功能
`GroupNorm(output, outputMean, outputVariance, inputX, gamma, beta, sharedTmpBuffer, epsilon, tiling)`：
输入 `[N, C, H*W]`，`C = G*D`，按 `(N, G)` 组在 `D*HW` 个元素上归一化；gamma/beta 为 per-channel `[C]`。
`y = (x - mean)/sqrt(var + eps) * gamma + beta`。

## 关键：填 GroupNormTiling（3510 用字段很少）
`AscendC::tiling::GroupNormTiling` 有 30 字段，但本 arch(3510) 的 impl 只读 `n/g/d/hw/factor`（核心 compute 直接用），
`dhwAlignSize` 仅用于选 ShapeScope 分支，`oneTmpSize` 仅被断言 `>0`：
```cpp
AscendC::tiling::GroupNormTiling t;
t.n=N; t.c=C; t.g=G; t.d=D; t.hw=HW;
t.dhwAlignSize = D*HW;          // <= oneRepSize(=GetVecLen()/4) → ShapeScope ONE
t.factor = 1.0f/(D*HW);         // mean 系数 1/(D*HW)
t.oneTmpSize = 256;             // 仅需 > 0
AscendC::GroupNorm<float>(z, mean, var, x, gamma, beta, tmp, 1e-5f, t);
```
对比 LayerNorm（regbase，需反推 k2Rec·k2RRec=1/R），GroupNorm 的 `factor` 直接就是 mean 系数，更直白。

## 最简 example 设计
- `[N=1, C=4, HW=16]`，`G=2, D=2`（每组 D*HW=32 元素），x 全 `5.0`，gamma 全 `1.0`，beta 全 `0.0`，eps `1e-5`。
- 组内全等 → mean=5、var=0 → output=(x−mean)·rstd·gamma+beta = `0`。单核，host 校验 errors=0。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
