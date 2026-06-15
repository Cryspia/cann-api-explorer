# Ascend C · SubReluCast (vector, fused conv)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: vector / fused sub + ReLU + cast
- Covered API: `SubReluCast`
- include: `kernel_operator.h`
- Source: CANN 9.1.0 Ascend C API Reference / SubReluCast

## Functionality
`SubReluCast` fuses element-wise subtract, ReLU, and a type conversion:
`dst = cast_to_T( relu(src0 - src1) )`. No dequant scale is needed (unlike `AddDeqRelu`).

## Measured signature
```cpp
template <typename T, typename U>
void SubReluCast(const LocalTensor<T>& dst, const LocalTensor<U>& src0,
                 const LocalTensor<U>& src1, const uint32_t count);
```
- Level-2 (count-mode) dtype combination on this SoC: `src(U) = float`, `dst(T) = half`
  (the impl also lists `<float,int64>` and `<int32,int64>` combos).

## Minimal example design
- `N = 256`. `src0[i]` alternates `-1 / 5`, `src1 = 2` (float).
- even `i`: `relu(-1 - 2) = relu(-3) = 0`; odd `i`: `relu(5 - 2) = relu(3) = 3`, cast to half.
- Host decodes half and verifies `0` / `3`. See `kernel.cpp` / `main.cpp`, results in `RESULT.md`.
