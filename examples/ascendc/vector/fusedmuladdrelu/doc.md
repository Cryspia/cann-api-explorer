# Ascend C · FusedMulAddRelu (skipped — deprecated alias of MulAddRelu)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Status: **skipped (alias)** — no `meta.json`, not counted as a separate unit.
- Covered by: the existing `muladdrelu` unit (`vector/muladdrelu`).

## Why skipped
In `basic_api/kernel_operator_vec_binary_intf.h`, `FusedMulAddRelu` carries the explicit header comment:
```cpp
// FusedMulAddRelu has been updated, please use MulAddRelu instead.
template <typename T>
__aicore__ inline void FusedMulAddRelu(const LocalTensor<T>& dst, const LocalTensor<T>& src0, ...);
```
The signature is identical to `MulAddRelu`; it is a renamed-deprecated alias of the same
operation, already verified by the `muladdrelu` unit. No separate implementation is added here.
