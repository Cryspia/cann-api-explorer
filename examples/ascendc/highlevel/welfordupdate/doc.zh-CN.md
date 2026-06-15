# Ascend C · WelfordUpdate（高阶，normalization）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / normalization
- 覆盖 API：`WelfordUpdate`（Welford 在线更新步）
- include：`lib/normalization/layernorm.h`
- 来源：CANN 9.1.0 Ascend C API Reference / WelfordUpdate

## 功能
`WelfordUpdate` 执行 Welford 算法的一次在线更新：把新样本 `x` 折叠进累积的 mean/variance。
它与已有的 `welfordfinalize` 单元互补（后者在结尾合并各分块累积量）。

3510 float 路径的逐元素公式（来自 `welford_3510_impl.h`）：
```
tmp     = x - inMean
outMean = inMean + nRec * tmp            (nRec = 1/n，累积计数的倒数)
outVar  = inVar  + tmp * (x - outMean)   (运行 M2 累积)
```

## 实测签名
```cpp
template <typename T, typename U, bool isReuseSource = false,
          const WelfordUpdateConfig& config = WFUPDATE_DEFAULT_CFG>
void WelfordUpdate(const LocalTensor<U>& outputMean, const LocalTensor<U>& outputVariance,
                   const LocalTensor<U>& inputMean, const LocalTensor<U>& inputVariance,
                   const LocalTensor<T>& inputX, const WelfordUpdateParam& para);
// 另有免 tmp 重载（本例使用）与 sharedTmpBuffer 重载。

struct WelfordUpdateParam { uint32_t rnLength; uint32_t abLength; uint32_t abComputeLength; float nRec; };
```
- `T` 支持 `half`/`bfloat16`/`float`；`U` 恒为 `float`。
- `para.abComputeLength`（= K）是处理的元素数；`para.nRec = 1/n`。

## 最简示例设计
- 单 tile，`ELEM = 8` 个 float（32B）。`abLength = abComputeLength = 8`。
- `inMean = 1`、`inVar = 0`、`x = 3`、`nRec = 0.5`（n = 2）：
  `outMean = 1 + 0.5*(3-1) = 2`，`outVar = 0 + (3-1)*(3-2) = 2`。
- host 校验 `outMean == 2` 且 `outVar == 2`。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
