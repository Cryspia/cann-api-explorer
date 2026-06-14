# Ascend C · Where (high-level, element-wise conditional select)

- Category: high-level adv_api / math
- Covered API: `Where`
- include: `lib/math/where.h`
- Source: CANN 9.1.0 Ascend C API Reference / Where

## Functionality
`Where(dst, src0, src1, condition, count)`: element-wise conditional select,
`dst[i] = condition[i] ? src0[i] : src1[i]`.

- `condition` is a `LocalTensor<bool>` (1 byte per element).
- `src0` / `src1` may each be a `LocalTensor` or a scalar; `dst` and the tensor operands share the value dtype.
- Supported value dtypes include `bool/int8/uint8/int16/uint16/half/bfloat16/int32/uint32/float/int64/uint64`.
- Availability: the device path is guarded by `__NPU_ARCH__ == 3510 || 5102`. Ascend950PR (the simulator's `dav_3510`) maps to 3510, so it builds and runs here.

## Minimal example design
- `count = 512`, dtype `float`. `src0 = 1.0`, `src1 = 2.0`.
- `condition[i] = (i even)` -> picks src0 on even indices, src1 on odd: `dst[i] = (i even) ? 1.0 : 2.0`.
- This exercises the tensor/tensor branch of `Where` and is fully host-verifiable (tol 1e-3).
- Single-core execution. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
