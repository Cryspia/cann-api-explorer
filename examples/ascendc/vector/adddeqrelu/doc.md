# Ascend C · AddDeqRelu (vector, fused conv with dequant)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: vector / fused add + dequant + ReLU
- Covered API: `AddDeqRelu`
- include: `kernel_operator.h`
- Source: CANN 9.1.0 Ascend C API Reference / AddDeqRelu

## Functionality
`AddDeqRelu` fuses element-wise add, dequant, and ReLU:
`dst = relu( (src0 + src1) * deqScale )`.

The dequant scale is the **scalar `g_deqValue`** set via `SetDeqScale(half)`. The 3510 impl
computes `max( float(src0+src1) * (1/131072) * g_deqValue * 131072, 0 )`, where the
`1/131072` and `131072` factors cancel, leaving `max((src0+src1)*deqScale, 0)`. This is the
simplest deterministic, checkable dequant path (the per-channel `SetDeqScale(LocalTensor, VdeqInfo)`
form uses a bit-packed `g_deqScale` and is not used here).

## Measured signature
```cpp
void AddDeqRelu(const LocalTensor<half>& dst, const LocalTensor<int32_t>& src0,
                const LocalTensor<int32_t>& src1, const int32_t& count);
// (a generic <T,U> overload also exists)
```
- src0/src1 are `int32_t`; dst is `half`. `count` is `int32_t`.
- `SetDeqScale(half scale)` must be called before the op to set the scalar dequant value.

## Minimal example design
- `N = 256`, `deqScale = 2.0`. src0/src1 (int32) alternate:
  even `i`: `(1 + 2) * 2 = 6`; odd `i`: `(-5 + 1) * 2 = -8` → relu → `0`.
- Host decodes half and verifies `6` / `0`. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
