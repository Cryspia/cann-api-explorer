# Ascend C · CastDeq（跳过 —— CastDequant 的废弃别名）

> English: [doc.md](doc.md)

- 状态：**跳过（别名）** —— 无 `meta.json`，不计为独立单元。
- 已由：`castdequant` 单元（`vector/castdequant`）覆盖。

## 跳过原因
在 `interface/kernel_operator_vec_vconv_intf.h` 中，`CastDeq` 带有明确的头注释：
```cpp
// CastDeq has been updated, please use CastDequant instead.
template <typename T, typename U, bool isVecDeq = true, bool halfBlock = true>
__aicore__ inline void CastDeq(const LocalTensor<T>& dst, const LocalTensor<U>& src, const uint32_t count);
```
其签名与 `CastDequant` 完全一致，是同一操作的重命名废弃别名，已由 `castdequant` 单元校验。
此处不另建实现。
