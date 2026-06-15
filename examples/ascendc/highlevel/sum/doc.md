# Ascend C · Sum
- highlevel / last-axis reduction　- `AscendC::Sum`　- include: `lib/reduce/sum.h`
- `Sum<T>(dst, src, sharedTmpBuffer, SumParams{outter, inner, n})`: reduces each of `outter` rows (row stride `inner`, valid length `n`) to one sum.
- Constraints: `1 <= n <= inner` and `inner*sizeof(T)` a multiple of 32.
- Example: shape [1,64], n=64, inner=64, src all 1.0 -> dst[0] = 64.0. errors=0.
