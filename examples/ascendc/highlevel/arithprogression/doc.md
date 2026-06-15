# Ascend C · ArithProgression
- highlevel / index sequence　- `AscendC::Arange`　- include: `lib/index/arithprogression.h`
- `Arange<T>(dst, firstValue, diffValue, count)`: **dst[i] = firstValue + i \* diffValue**.
- Example: firstValue=0, diffValue=1 -> dst[i] = i (0,1,2,...,63). errors=0.
- Note: the adv_api `arithprogression` library is exposed through the `Arange` API; no input tensor or tmp buffer is needed.
