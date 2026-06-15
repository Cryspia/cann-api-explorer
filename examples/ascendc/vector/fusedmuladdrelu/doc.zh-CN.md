# Ascend C · FusedMulAddRelu（跳过 —— MulAddRelu 的废弃别名）

> English: [doc.md](doc.md)

- 状态：**跳过（别名）** —— 无 `meta.json`，不计为独立单元。
- 已由：`muladdrelu` 单元（`vector/muladdrelu`）覆盖。

## 跳过原因
在 `basic_api/kernel_operator_vec_binary_intf.h` 中，`FusedMulAddRelu` 带有明确的头注释：
```cpp
// FusedMulAddRelu has been updated, please use MulAddRelu instead.
template <typename T>
__aicore__ inline void FusedMulAddRelu(const LocalTensor<T>& dst, const LocalTensor<T>& src0, ...);
```
其签名与 `MulAddRelu` 完全一致，是同一操作的重命名废弃别名，已由 `muladdrelu` 单元校验。
此处不另建实现。
