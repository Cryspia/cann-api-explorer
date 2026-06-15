# Ascend C · FusedExpSub（跳过 —— ExpSub 的废弃别名）

> English: [doc.md](doc.md)

- 状态：**跳过（别名）** —— 无 `meta.json`，不计为独立单元。
- 已由：`expsub` 单元（`vector/expsub`）覆盖。

## 跳过原因
在 `basic_api/kernel_operator_vec_binary_intf.h` 中，`FusedExpSub` 带有明确的头注释：
```cpp
// FusedExpSub has been updated, please use ExpSub instead.
template <typename T, typename U>
__aicore__ inline void FusedExpSub(const LocalTensor<T> &dst, const LocalTensor<U> &src0,
    const LocalTensor<U> &src1, const uint32_t count);
```
其签名与 `ExpSub`（`dst = e^(src0 - src1)`）完全一致，是同一操作的重命名废弃别名，已由 `expsub`
单元校验。此处不另建实现。
