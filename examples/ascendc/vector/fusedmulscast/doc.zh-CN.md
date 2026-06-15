# Ascend C · FusedMulsCast（跳过 —— MulsCast 的废弃别名）

> English: [doc.md](doc.md)

- 状态：**跳过（别名）** —— 无 `meta.json`，不计为独立单元。
- 已由：`mulscast` 单元（`vector/mulscast`）覆盖。

## 跳过原因
在 `basic_api/kernel_operator_vec_binary_scalar_intf.h` 中，`FusedMulsCast` 带有明确的头注释：
```cpp
// FusedMulsCast has been updated, please use MulsCast instead.
template <typename T1, typename T2, typename T3, typename T4>
__aicore__ inline void FusedMulsCast(const T2 &dst, const T3 &src0, const T4 &src1, const uint32_t count);
```
其签名与 `MulsCast` 完全一致（相同模板参数、相同 `(dst, src0, src1, count)` 形式），是同一操作的
重命名废弃别名。其功能（`dst = cast(src0 * scalar)`）已由 `mulscast` 单元校验，因此此处不另建可校验实现。
