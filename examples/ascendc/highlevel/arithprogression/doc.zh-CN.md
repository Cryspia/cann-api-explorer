# Ascend C · ArithProgression
- highlevel / 等差数列索引　- `AscendC::Arange`　- include：`lib/index/arithprogression.h`
- `Arange<T>(dst, firstValue, diffValue, count)`：**dst[i] = firstValue + i \* diffValue**。
- 例：firstValue=0，diffValue=1 -> dst[i] = i（0,1,2,...,63）。errors=0。
- 说明：adv_api 的 `arithprogression` 库通过 `Arange` 接口暴露；无需输入张量或 tmp。
