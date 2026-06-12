# Ascend C · LogSoftmax (high-level, with Tiling)

- Category: high-level adv_api / activation
- APIs covered: `AscendC::LogSoftMax`
- include: `lib/activation/logsoftmax.h` (+ `lib/activation/softmax.h` for the device `SoftMaxTilingFunc`)
- Source: CANN 9.1.0 Ascend C API Reference / LogSoftMax

## Function
`LogSoftMax<T>(dst, sum, max, src, tmp, tiling, softmaxShapeInfo)`:
`max=rowmax(x)`, `sum=rowsum(exp(x-max))`, `y=log(exp(x-max)/sum)=log(softmax(x))`, along the last dim K.

## Key point 1: reuse SoftMaxTilingFunc for tiling (no host tiling)
`AscendC::tiling::LogSoftMaxTiling` has **exactly the same struct layout** as `SoftMaxTiling` (the same 16 uint32: srcM/srcK/srcSize/outMax*/split*/reduce*/rangeM/tailM/tailSplitSize/tailReduceSize).
3510 takes the regbase path -> `SoftMaxGenericNDImpl`, which needs the full tiling, but there is no device-side `LogSoftMaxTilingFunc` (the host-side one requires `ge::Shape`).
Solution: inside the kernel, use the already-verified device `SoftMaxTilingFunc` to fill `SoftMaxTiling`, then copy field by field into `LogSoftMaxTiling`:
```cpp
AscendC::tiling::SoftMaxTiling smt;
AscendC::SoftMaxTilingFunc(TMP_BYTES/sizeof(DT), info, smt, sizeof(DT), sizeof(DT));
AscendC::tiling::LogSoftMaxTiling lst;
auto *s = (uint32_t*)&smt; auto *d = (uint32_t*)&lst;
for (uint32_t i=0;i<sizeof(lst)/4;i++) d[i]=s[i];
AscendC::LogSoftMax<float>(zL, sumL, maxL, xL, tmp, lst, info);
```

## Key point 2: this implementation's log is **log10** (measured, not the natural log)
In simulation, with x all 0 and K=64, `dst = -1.80618`, **exactly equal to `log10(1/64) = -6·log10(2)`**, not `-ln(64)=-4.1589`.
That is, this arch's `LogSoftMax` outputs `log10(softmax)`. The verification expected value must use `-log10f(K)`.

## Simplest example design
- `[M=8,K=64]`, `x` all `0` -> softmax=1/64 -> `dst = log10(1/64) ≈ -1.806180`. Single core, host verify errors=0 (instr≈514).
