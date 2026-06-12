# Ascend C · DeepNorm（高阶，5 字段手填 Tiling）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`AscendC::DeepNorm`
- include：`lib/normalization/deepnorm.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / DeepNorm

## 功能
`DeepNorm<T,isReuseSrc,isBasicBlock>(dst, mean, rstd, x, gx, beta, gamma, tmp, alpha, epsilon, tiling)`：
`y = LayerNorm(alpha*x + gx) * gamma + beta`，mean/var 在 H 维上计算。`gx` 即 Sublayer(x) 残差项。

## 关键：3510 只读 5 个形状字段，mean 系数内部算
`DeepNormTiling` 有 20+ 字段，3510 impl 只引用 `bLength / sLength / hLength / originalHLength / oneTmpSize`：
- `GetDeepnormPara` 用 `hLength` 算 `hRepeatTimes/hTailSize/hDim`；`para.hDim = hLength` 即 mean 的除数，**无需反推系数**（对比 LayerNorm 要反推 `k2Rec·k2RRec=1/R`，DeepNorm 直白得多）。
- `alpha` 经 `Reg::Axpy` 实现 `alpha*x + gx`。
- `originalHLength` 仅用于 `isBasicBlock` 判断（H%64==0 等）；`oneTmpSize` 仅断言 `>0`。
```cpp
AscendC::tiling::DeepNormTiling t;
t.bLength = 1; t.sLength = 1; t.hLength = 8; t.originalHLength = 8; t.oneTmpSize = 256;
AscendC::DeepNorm<float, false, false>(zL, meanL, rstdL, xL, gxL, betaL, gammaL, tmp, /*alpha*/2.0f, /*eps*/1e-5f, t);
```

## 最简 example 设计
- `[B=1,S=1,H=8]`，`x=1, gx=0, alpha=2 → eff=alpha*x+gx=2` 全相等 → `var=0`。
- `gamma=1, beta=3` → `dst = 0*gamma + beta = 3`，`mean=2`。单核，host 校验 errors=0（instr≈854）。
