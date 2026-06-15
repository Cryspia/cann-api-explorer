# Ascend C · Mean
- highlevel / 末轴归约　- `AscendC::Mean`　- include：`lib/reduce/mean.h`
- `Mean<T>(dst, src, sharedTmpBuffer, MeanParams{outter, inner, n})`：对 `outter` 行（行跨度 `inner`，有效长度 `n`）分别求均值。
- 约束：`1 <= n <= inner`，且 `inner*sizeof(T)` 为 32 的倍数。
- 例：shape [1,64]，n=64，inner=64，src 全 2.0 -> dst[0] = 2.0。errors=0。
