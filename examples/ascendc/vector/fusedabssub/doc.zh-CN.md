# Ascend C · FusedAbsSub（跳过 —— AbsSub 的废弃别名）

> English: [doc.md](doc.md)

- 状态：**跳过（别名）** —— 无 `meta.json`，不计为独立单元。
- 已由：`abssub` 单元（`vector/abssub`）覆盖。

## 跳过原因
在 `basic_api/kernel_operator_vec_binary_intf.h` 中，`FusedAbsSub` 带有明确的头注释：
```cpp
// FusedAbsSub has been updated, please use AbsSub instead.
template <typename T>
__aicore__ inline void FusedAbsSub(const LocalTensor<T> &dst, const LocalTensor<T> &src0,
    const LocalTensor<T> &src1, const uint32_t count);
```
其签名与 `AbsSub`（`dst = abs(src0 - src1)`）完全一致，是同一操作的重命名废弃别名，已由 `abssub`
单元校验。此处不另建实现。
