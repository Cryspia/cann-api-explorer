# Ascend C · ExpSub
- 矢量 / 差值取指数　- `AscendC::ExpSub`　- include：`kernel_operator.h`
- `ExpSub<T,U>(dst, src0, src1, count)`：**dst[i] = exp(src0[i] - src1[i])**（src 为 half/float，dst 为 float）。
- 例：src0=1，src1=1 → dst = exp(0) = 1（tol 1e-2）。errors=0。
