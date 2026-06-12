# Ascend C · LogSoftmax（高阶，带 Tiling）

- 分类：高阶 adv_api / 激活
- 覆盖 API：`AscendC::LogSoftMax`
- include：`lib/activation/logsoftmax.h`（+ `lib/activation/softmax.h` 取 device `SoftMaxTilingFunc`）
- 原文：CANN 9.1.0 Ascend C API 参考 / LogSoftMax

## 功能
`LogSoftMax<T>(dst, sum, max, src, tmp, tiling, softmaxShapeInfo)`：
`max=rowmax(x)`，`sum=rowsum(exp(x-max))`，`y=log(exp(x-max)/sum)=log(softmax(x))`，沿最后一维 K。

## 关键 1：tiling 复用 SoftMaxTilingFunc（免 host tiling）
`AscendC::tiling::LogSoftMaxTiling` 与 `SoftMaxTiling` **结构布局完全相同**（同样 16 个 uint32：srcM/srcK/srcSize/outMax*/split*/reduce*/rangeM/tailM/tailSplitSize/tailReduceSize）。
3510 走 regbase → `SoftMaxGenericNDImpl`，需完整 tiling，但无 device 端 `LogSoftMaxTilingFunc`（host 端那个要 `ge::Shape`）。
解法：kernel 内用已验证的 device `SoftMaxTilingFunc` 填 `SoftMaxTiling`，再逐字段拷给 `LogSoftMaxTiling`：
```cpp
AscendC::tiling::SoftMaxTiling smt;
AscendC::SoftMaxTilingFunc(TMP_BYTES/sizeof(DT), info, smt, sizeof(DT), sizeof(DT));
AscendC::tiling::LogSoftMaxTiling lst;
auto *s = (uint32_t*)&smt; auto *d = (uint32_t*)&lst;
for (uint32_t i=0;i<sizeof(lst)/4;i++) d[i]=s[i];
AscendC::LogSoftMax<float>(zL, sumL, maxL, xL, tmp, lst, info);
```

## 关键 2：该实现的 log 是 **log10**（实测，非自然对数）
仿真实测 x 全 0、K=64 时 `dst = -1.80618`，**精确等于 `log10(1/64) = -6·log10(2)`**，而非 `-ln(64)=-4.1589`。
即本 arch 的 `LogSoftMax` 输出 `log10(softmax)`。校验期望须用 `-log10f(K)`。

## 最简 example 设计
- `[M=8,K=64]`，`x` 全 `0` → softmax=1/64 → `dst = log10(1/64) ≈ -1.806180`。单核，host 校验 errors=0（instr≈514）。
