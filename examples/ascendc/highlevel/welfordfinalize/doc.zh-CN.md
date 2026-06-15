# Ascend C · WelfordFinalize（高阶，统计/归一化）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / normalization
- 覆盖 API：`WelfordFinalize`
- include：`lib/normalization/welfordfinalize.h`
- 来源：CANN 9.1.0 Ascend C API 参考 / WelfordFinalize

## 函数
```
WelfordFinalize<isReuseSource, config>(
    outputMean,       // LocalTensor<float>，标量结果在下标 0
    outputVariance,   // LocalTensor<float>，标量结果在下标 0
    inputMean,        // LocalTensor<float>，形状 [abLength]（各分块的部分均值）
    inputVariance,    // LocalTensor<float>，形状 [abLength]（各分块的部分方差）
    sharedTmpBuffer,  // LocalTensor<uint8_t>
    para)             // WelfordFinalizePara
```
把分块 Welford 过程产出的各分块（部分）均值与方差，最终化合并为单一的最终均值与方差。
另有带额外 `counts`（`LocalTensor<int32_t>`，按各分块元素数加权）的重载，以及不带
`sharedTmpBuffer`（内部自动 PopStackBuffer）的重载。数据全为 `float`。

## 要点：WelfordFinalizePara 与最简归约路径
```cpp
struct WelfordFinalizePara {
    uint32_t rnLength, abLength, headCount, headCountLength, tailCount, tailCountLength;
    float abRec, rRec, rRecWithCorrection;   // rRecWithCorrection 仅 3510/5102
};
```
约束：`abLength = headCountLength + tailCountLength`，且 `abLength` 必须 32B 对齐。
最简分支令 `tailCountLength = 0`（`tailCount` 保持非零，以满足 debug 断言
“tailCount 为 0 时 tailCountLength 不可为 0”）。在本 arch 上该分支计算：
```
outMean = abRec * sum(inputMean)
outVar  = rRec  * sum(inputVariance + rnLength * (inputMean - outMean)^2)
```
取 `abRec = rRec = 1/abLength` 即为普通平均。

## 最简样例设计
- `abLength = 8`（32B 对齐，且 `< 64`，归约走简单单块路径）。
- `inputMean` 全 `4.0` -> `outMean = (1/8) * (8*4) = 4.0`。
- `inputVariance` 全 `9.0`；由于每个部分均值都等于 `outMean`，`(inputMean - outMean) = 0`，
  故 `outVar = (1/8) * (8*9) = 9.0`（`rnLength` 项消失）。
- 输出为标量，从各输出缓冲下标 `0` 读取。单核运行，host 校验 `errors=0`。
- 详见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
