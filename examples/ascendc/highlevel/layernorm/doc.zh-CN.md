# Ascend C · LayerNorm（高阶，带 Tiling，本 arch=3510 regbase）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`LayerNorm`（regbase `LayerNorm(Para, LayerNormSeparateTiling)` 重载）
- include：`lib/normalization/layernorm.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / LayerNorm

## 功能
`y = (x - mean) / sqrt(var + eps) * gamma + beta`，沿最后一维 R 归一化。
输出 `output[A,R]` + `outputMean[A]` + `outputRstd[A]`。

## 关键：本 arch(3510) 走 regbase，必须用 Para + SeparateTiling
标准 `LayerNorm(..., LayerNormTiling)` 重载只在 arch 2002/2201 提供。**3510 只暴露**
`LayerNorm<U,T>(out, mean, rstd, x, gamma, beta, eps, tmp, LayerNormPara, LayerNormSeparateTiling)`。
取 `rLength <= sregLower`(=GetVecLen()/4，本机 64) 的简单分支后，只需手填少数字段：
```cpp
AscendC::LayerNormPara para;            // {aLength=A, rLength=R, rLengthWithPadding=R}
AscendC::tiling::LayerNormSeparateTiling t;  // 其余字段 0 即可
t.aLength=A; t.rLength=R; t.k2Rec=1.0f; t.k2RRec=1.0f/R;
AscendC::LayerNorm<float,float>(z, mean, rstd, x, gamma, beta, 1e-5f, tmp, para, t);
```

## 踩坑记录（关键）
mean 的 device 实现是 `mean = k2RRec · ReduceSum(k2Rec · x)`，即 **k2Rec × k2RRec 必须 = 1/R**。
初版把两者都设成 `1/R` → 乘积 `1/R²` → mean=192/4096=0.047 → output=(3−0.047)·rstd≈**7.9997**（仿真不崩溃但数值全错）。
改成 `k2Rec=1.0, k2RRec=1/R`（乘积 1/R）后 mean=3、var=0、output=0 ✓。
（regbase 的 k2Rec/k2RRec/rHeadLength/oneTmpSize 等系数在生产中由 host tiling 计算，shipped 头不含公式，需读 device impl 反推。）

## 最简 example 设计
- `[A,R]=[8,64]`，x 全 `3.0`，gamma 全 `1.0`，beta 全 `0.0`，eps `1e-5`。
- 每行 mean=3、var=0 → output=(x−mean)·rstd·gamma+beta = `0`。单核，host 校验 errors=0。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。指令执行数 909。
