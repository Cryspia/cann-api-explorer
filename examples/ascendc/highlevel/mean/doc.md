# Ascend C · Mean
- highlevel / last-axis reduction　- `AscendC::Mean`　- include: `lib/reduce/mean.h`
- `Mean<T>(dst, src, sharedTmpBuffer, MeanParams{outter, inner, n})`: reduces each of `outter` rows (row stride `inner`, valid length `n`) to its average.
- Constraints: `1 <= n <= inner` and `inner*sizeof(T)` a multiple of 32.
- Example: shape [1,64], n=64, inner=64, src all 2.0 -> dst[0] = 2.0. errors=0.
