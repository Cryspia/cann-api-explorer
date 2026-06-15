# Ascend C · FusedAbsSub (skipped — deprecated alias of AbsSub)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Status: **skipped (alias)** — no `meta.json`, not counted as a separate unit.
- Covered by: the existing `abssub` unit (`vector/abssub`).

## Why skipped
In `basic_api/kernel_operator_vec_binary_intf.h`, `FusedAbsSub` carries the explicit header comment:
```cpp
// FusedAbsSub has been updated, please use AbsSub instead.
template <typename T>
__aicore__ inline void FusedAbsSub(const LocalTensor<T> &dst, const LocalTensor<T> &src0,
    const LocalTensor<T> &src1, const uint32_t count);
```
The signature is identical to `AbsSub` (`dst = abs(src0 - src1)`); it is a renamed-deprecated
alias of the same operation, already verified by the `abssub` unit. No separate implementation
is added here.
