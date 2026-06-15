# Ascend C · Normalize（高阶，统计/归一化）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / normalization
- 覆盖 API：`Normalize`
- include：`lib/normalization/normalize.h`
- 来源：CANN 9.1.0 Ascend C API 参考 / Normalize

## 函数
```
Normalize<U, T, isReuseSource, config>(
    output,           // LocalTensor<T>,     形状 [A, R]
    outputRstd,       // LocalTensor<float>, 形状 [A]
    inputMean,        // LocalTensor<float>, 形状 [A]
    inputVariance,    // LocalTensor<float>, 形状 [A]
    inputX,           // LocalTensor<T>,     形状 [A, R]
    gamma,            // LocalTensor<U>,     形状 [R]
    beta,             // LocalTensor<U>,     形状 [R]
    sharedTmpBuffer,  // LocalTensor<uint8_t>
    epsilon,          // float
    para)             // NormalizePara { aLength, rLength, rLengthWithPadding }
```
给定每行预先算好的均值与方差，Normalize 相当于 LayerNorm 的“后半段”：
```
rstd   = rsqrt(variance + epsilon)
output = (x - mean) * rstd * gamma + beta
```
`outputRstd` 恒为 `float`。另有一个不带 `sharedTmpBuffer` 的重载（内部自动 PopStackBuffer 取临时空间）。

## 要点：无需 host tiling，只填 NormalizePara
在本 arch（3510）impl 只读 `para.{aLength, rLength, rLengthWithPadding}`：
```cpp
AscendC::NormalizePara para;
para.aLength = A;                 // 行数
para.rLength = R;                 // 归约长度
para.rLengthWithPadding = RPAD;   // 32B 对齐的行步长
AscendC::Normalize<float, float>(y, rstd, mean, var, x, gamma, beta, tmp, eps, para);
```
默认 `NormalizeConfig` 为 `AR` 归约模式，且应用 gamma 与 beta。

## 最简样例设计
- `[A=2, R=8]`，`RPAD=8`。mean `0`，variance `1`，eps `0` -> `rstd = rsqrt(1) = 1`。
- x 全 `1.0`，gamma 全 `1.0`，beta 全 `0.0` -> `output = (1-0)*1*1+0 = 1`，`outputRstd = 1`。
- `[A]` 形状的张量（mean/variance/rstd）按 8 个 float（32B）填充存储，使 `DataCopy` 32B 对齐；填充位的 `variance` 仍设 `1`，避免广播路径里出现 `rsqrt(0)`。
- `outputRstd` 走独立的 `VECOUT` 队列，保证向量流水线写入在 MTE3 拷出前完成（否则竞争会读回 `0`）。
- 单核运行，host 校验 `errors=0`。详见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
