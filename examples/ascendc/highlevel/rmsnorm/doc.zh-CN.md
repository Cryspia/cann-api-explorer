# Ascend C · RmsNorm（高阶，带 Tiling）

- 分类：高阶 adv_api / 归一化
- 覆盖 API：`RmsNorm`
- include：`lib/normalization/rmsnorm.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / RmsNorm

## 功能
`RmsNorm(dst, src, gamma, sharedTmpBuffer, epsilon, tiling)`：
`y = x / sqrt(mean(x^2) + eps) * gamma`，沿最后一维 H 归一化。

## 关键：kernel 内手工填充 Tiling
RmsNorm **没有** device 端 tiling func（不像 SoftMax）。但 `AscendC::tiling::RmsNormTiling` 只有 12 个字段，
固定单 tile 形状 `[B,S,H]=[1,8,64]`（一次循环、无 tail）下可在 kernel 内安全手填：
```cpp
AscendC::tiling::RmsNormTiling t;
t.bLength=B; t.sLength=S; t.hLength=H; t.originalHLength=H;
t.reciprocalOfHLength=1.0f/H;
t.mainBshLength=B*S*H; t.mainBsLength=B*S; t.mainBsLengthAlign=B*S;
t.loopRound=1; t.inputTailPos=0; t.tailBshLength=0; t.tailBsLength=0;
AscendC::RmsNorm<float>(zL, xL, gammaL, tmp, 1e-5f, t);
```
（生产环境多 tile/大 shape 时这些字段应由 host 侧 tiling 计算得出。）

## 最简 example 设计
- `[1,8,64]`，x 全 `2.0`，gamma 全 `1.0`，eps `1e-5`。
- rms = sqrt(mean(4)+eps) ≈ 2.0 → y = 2/2*1 ≈ `1.0`。单核，host 校验（tol 5e-3）。
- 见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
