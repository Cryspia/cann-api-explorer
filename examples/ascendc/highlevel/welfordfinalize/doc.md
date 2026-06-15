# Ascend C · WelfordFinalize (high-level, statistics/normalization)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / normalization
- Covered API: `WelfordFinalize`
- include: `lib/normalization/welfordfinalize.h`
- Source: CANN 9.1.0 Ascend C API Reference / WelfordFinalize

## Function
```
WelfordFinalize<isReuseSource, config>(
    outputMean,       // LocalTensor<float>, scalar result at index 0
    outputVariance,   // LocalTensor<float>, scalar result at index 0
    inputMean,        // LocalTensor<float>, shape [abLength]  (per-partition partial means)
    inputVariance,    // LocalTensor<float>, shape [abLength]  (per-partition partial variances)
    sharedTmpBuffer,  // LocalTensor<uint8_t>
    para)             // WelfordFinalizePara
```
Combines the per-partition (partial) means and variances produced by a chunked Welford pass
into a single final mean and variance. There are also overloads that take an extra `counts`
`LocalTensor<int32_t>` (weight each partition by its element count), and overloads without
`sharedTmpBuffer` (a stack buffer is popped internally). All data is `float`.

## Key point: WelfordFinalizePara and the simplest reduce path
```cpp
struct WelfordFinalizePara {
    uint32_t rnLength, abLength, headCount, headCountLength, tailCount, tailCountLength;
    float abRec, rRec, rRecWithCorrection;   // rRecWithCorrection only on 3510/5102
};
```
Requirement: `abLength = headCountLength + tailCountLength` and `abLength` must be 32B aligned.
The simplest branch sets `tailCountLength = 0` (`tailCount` kept non-zero so the debug assert
"tailCountLength cannot be zero when tailCount is zero" is satisfied). On this arch that branch computes:
```
outMean = abRec * sum(inputMean)
outVar  = rRec  * sum(inputVariance + rnLength * (inputMean - outMean)^2)
```
Choosing `abRec = rRec = 1/abLength` makes them plain averages.

## Minimal example design
- `abLength = 8` (32B aligned, and `< 64` so the reduce takes the simple single-block path).
- All `inputMean = 4.0` -> `outMean = (1/8) * (8*4) = 4.0`.
- All `inputVariance = 9.0`; since every partial mean equals `outMean`, `(inputMean - outMean) = 0`,
  so `outVar = (1/8) * (8*9) = 9.0` (the `rnLength` term vanishes).
- Outputs are scalar, read from index `0` of each output buffer. Single core, host verifies `errors=0`.
- See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
