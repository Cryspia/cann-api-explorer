# Ascend C · FusedMulsCast (skipped — deprecated alias of MulsCast)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Status: **skipped (alias)** — no `meta.json`, not counted as a separate unit.
- Covered by: the existing `mulscast` unit (`vector/mulscast`).

## Why skipped
In `basic_api/kernel_operator_vec_binary_scalar_intf.h`, `FusedMulsCast` carries the explicit
header comment:
```cpp
// FusedMulsCast has been updated, please use MulsCast instead.
template <typename T1, typename T2, typename T3, typename T4>
__aicore__ inline void FusedMulsCast(const T2 &dst, const T3 &src0, const T4 &src1, const uint32_t count);
```
Its signature is identical to `MulsCast` (same template parameters, same `(dst, src0, src1, count)`
form); it is a renamed-deprecated alias of the same operation. The functionality
(`dst = cast(src0 * scalar)`) is already verified by the `mulscast` unit, so no separate
checkable implementation is added here.
