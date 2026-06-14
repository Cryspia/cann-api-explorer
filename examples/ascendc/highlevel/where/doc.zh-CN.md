# Ascend C · Where（高阶，逐元素条件选择）

- 分类：高阶 adv_api / math
- 覆盖 API：`Where`
- include：`lib/math/where.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / Where

## 功能
`Where(dst, src0, src1, condition, count)`：逐元素条件选择，
`dst[i] = condition[i] ? src0[i] : src1[i]`。

- `condition` 为 `LocalTensor<bool>`（每元素 1 字节）。
- `src0` / `src1` 可各为 `LocalTensor` 或标量；`dst` 与张量入参共用值 dtype。
- 支持的值 dtype 包括 `bool/int8/uint8/int16/uint16/half/bfloat16/int32/uint32/float/int64/uint64`。
- 可用性：device 实现以 `__NPU_ARCH__ == 3510 || 5102` 为条件。Ascend950PR（仿真器的 `dav_3510`）对应 3510，因此本机可编译并运行。

## 最简 example 设计
- `count = 512`，dtype `float`。`src0 = 1.0`，`src1 = 2.0`。
- `condition[i] = (i 为偶数)` → 偶数位取 src0、奇数位取 src1：`dst[i] = (i 偶) ? 1.0 : 2.0`。
- 覆盖 `Where` 的张量/张量分支，host 端可完整校验（tol 1e-3）。
- 单核执行。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
