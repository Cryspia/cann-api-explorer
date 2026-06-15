# Ascend C · LayerNormGradBeta（高阶 adv_api）

- 类别：高阶 / normalization（反向）
- 覆盖接口：`AscendC::LayerNormGradBeta<T>(outputPdGamma, outputPdBeta, resForGamma, inputDy, sharedTmpBuffer, LayerNormGradBetaTiling)`
- 头文件：`lib/normalization/layernormgradbeta.h`（kernel）
- 状态：在本机 3510（Ascend950PR）无卡仿真上 **passed**。

## 计算内容
这是 LayerNorm 反向的第二阶段：沿 `(B, S)` 轴规约，得到 gamma/beta 的梯度（输出形状 `[H]`）：

```
pd_gamma[h] = sum_bs( inputDy[bs, h] * resForGamma[bs, h] )
pd_beta[h]  = sum_bs( inputDy[bs, h] )
```

`resForGamma` 是 `LayerNormGrad` 产出的 `[B, S, H]` tensor；`inputDy` 是同形状的上游梯度。

## 3510 上的 tiling
3510 实现只读取 `tiling.bsLength` 和 `tiling.hLength`，并要求 `hLength * sizeof(T)` 是 32 字节的整数倍
（此处 `8 * 4 = 32`，满足）。

## 设计
取一个非平凡但可精确校验的算例（无需把梯度置零）：

- 形状 `[BS = 4, H = 8]`，`dtype = float`。
- `inputDy = 2.0`，`resForGamma = 3.0`（全常量）。
- `pd_gamma[h] = 4 * (2 * 3) = 24.0`，`pd_beta[h] = 4 * 2 = 8.0`。已在 host 校验。

## 运行
```
bash harness/run_one.sh examples/ascendc/highlevel/layernormgradbeta
```
