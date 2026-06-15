# Ascend C · CastDeq (skipped — deprecated alias of CastDequant)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Status: **skipped (alias)** — no `meta.json`, not counted as a separate unit.
- Covered by: the `castdequant` unit (`vector/castdequant`).

## Why skipped
In `interface/kernel_operator_vec_vconv_intf.h`, `CastDeq` carries the explicit header comment:
```cpp
// CastDeq has been updated, please use CastDequant instead.
template <typename T, typename U, bool isVecDeq = true, bool halfBlock = true>
__aicore__ inline void CastDeq(const LocalTensor<T>& dst, const LocalTensor<U>& src, const uint32_t count);
```
The signature is identical to `CastDequant`; it is a renamed-deprecated alias of the same
operation, already verified by the `castdequant` unit. No separate implementation is added here.
