# Ascend C · Select (+ CompareScalar)

- Category: vector compute / compare-select (hand-written combined unit)
- APIs covered: `CompareScalar`, `Select`
- Source: CANN 9.1.0 Ascend C API Reference / vector compare-select

## Functionality
- `CompareScalar(mask, src, scalar, cmpMode, count)`: per-element comparison, the result is written bit by bit into the uint8 mask `mask`.
- `Select(dst, mask, src0, src1, selMode, count)`: per-element selection from `src0`/`src1` according to `mask`.

## Function prototypes (count mode, from the toolkit header `kernel_operator_vec_cmpsel_intf.h`)
```cpp
void CompareScalar(const LocalTensor<U>& dst, const LocalTensor<T>& src0,
                   const T src1Scalar, CMPMODE cmpMode, uint32_t count);
void Select(const LocalTensor<T>& dst, const LocalTensor<U>& selMask,
            const LocalTensor<T>& src0, const LocalTensor<T>& src1,
            SELMODE selMode, uint32_t count);
```
- `CMPMODE`: LT/GT/EQ/LE/GE/NE; `SELMODE`: VSEL_TENSOR_TENSOR_MODE etc.
- The mask `U=uint8_t`, stored compactly bit by bit (count elements take count/8 bytes).

## Minimal example design
- x filled entirely with `1.0`, y filled entirely with `2.0`.
- `mask = CompareScalar(x > 0.0)` -> all true (1>0).
- `z = Select(mask, x, y, VSEL_TENSOR_TENSOR_MODE)` -> take x everywhere -> expect `z = 1.0`.
- This way only the float output of Select needs verification, avoiding bit-by-bit verification of the mask.
- See `kernel.cpp` / `main.cpp` in the same directory; results in `RESULT.md`.
