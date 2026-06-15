# Ascend C · FusedExpSub (skipped — deprecated alias of ExpSub)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Status: **skipped (alias)** — no `meta.json`, not counted as a separate unit.
- Covered by: the existing `expsub` unit (`vector/expsub`).

## Why skipped
In `basic_api/kernel_operator_vec_binary_intf.h`, `FusedExpSub` carries the explicit header comment:
```cpp
// FusedExpSub has been updated, please use ExpSub instead.
template <typename T, typename U>
__aicore__ inline void FusedExpSub(const LocalTensor<T> &dst, const LocalTensor<U> &src0,
    const LocalTensor<U> &src1, const uint32_t count);
```
The signature is identical to `ExpSub` (`dst = e^(src0 - src1)`); it is a renamed-deprecated
alias of the same operation, already verified by the `expsub` unit. No separate implementation
is added here.
