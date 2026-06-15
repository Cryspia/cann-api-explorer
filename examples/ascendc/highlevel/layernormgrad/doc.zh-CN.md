# Ascend C · LayerNormGrad（高阶 adv_api）

- 类别：高阶 / normalization（反向）
- 覆盖接口：`AscendC::LayerNormGrad<T>(outputPdX, resForGamma, inputDy, inputX, inputVariance, inputMean, inputGamma, sharedTmpBuffer, epsilon, LayerNormGradTiling)`
- 头文件：`lib/normalization/layernormgrad.h`（kernel）
- 状态：在本机 3510（Ascend950PR）无卡仿真上 **passed**。

## 计算内容
LayerNormGrad 是 LayerNorm 的反向，产出两个输出（形状 `[B, S, H]`）：

```
x1          = inputDy * inputGamma
x2          = inputX  - inputMean
rstd        = 1 / sqrt(inputVariance + epsilon)
pd_var      = sum_H( -0.5 * x1 * x2 * (var+eps)^(-1.5) )
pd_mean     = sum_H( -1.0 * x1 * rstd ) + pd_var * (1/H) * sum_H(-2 * x2)
resForGamma = x2 * rstd
outputPdX   = x1 * rstd + pd_var * (2/H) * x2 + pd_mean * (1/H)
```

`inputVariance`、`inputMean` 形状为 `[B, S, 1]`；`inputGamma` 形状为 `[H]`。

## 3510 上的 tiling
3510 实现只读取六个字段：
`stackBufferSize, bLength, sLength, hLength, lastDimValueBack, lastDimValueBackMulTwo`。
后两个是 `1/H` 和 `2/H`，以 **float 位模式** 存放在 `uint32` 字段里（实现端再
`reinterpret_cast` 回 `float`），所以 kernel 用 `reinterpret_cast<uint32_t*>(&floatValue)` 写入。

## 如何构造可校验的简单算例
没有 skip 反向，而是选取能把两个输出都驱动到已知值的输入：

- `dy = 0` -> `x1 = 0` -> `pd_var = 0`、`pd_mean = 0` -> `outputPdX` 全 0。
- `x = mean = 5.0`（于是 `var = 0`）-> `x2 = 0` -> `resForGamma` 全 0。

于是在 `[B=1, S=1, H=8]`、`gamma = 1.0`、`eps = 1e-5` 下，`outputPdX` 与 `resForGamma` 都是 `0`。
已在 host 校验。（该算例仍然走完整实现路径——load、pd_var/pd_mean 规约、pd_x 组装——
只是把期望值取成易校验的 0。）

## 运行
```
bash harness/run_one.sh examples/ascendc/highlevel/layernormgrad
```
