# Ascend C / BitwiseAnd

- Category: vector compute / highlevel (integer bitwise, adv_api simple count mode)
- dtype: integer; both src and dst share the same type `T`. Level-2 integer dtypes on this SoC: `int16`/`uint16`/`int64`/`uint64` (`int32` is NOT supported), so this unit uses `int16`.
- Relation: this is the **adv_api (`lib/math`) count-mode** entry for per-element bitwise AND. The identical operation is also exposed by the lower-level vector intrinsic `AscendC::And` (see `../../vector/and/`); same semantics, different API layer.
- Source: see the CANN 9.1.0 Ascend C API reference (`adv_api/math/bitwise_and.h`)

## Functionality
`BitwiseAnd`: element-wise bitwise AND of two integer tensors, `dst[i] = src0[i] & src1[i]`.

## Function prototype (count mode, taken from toolkit header `bitwise_and.h`, __NPU_ARCH__==3510)
```cpp
template <const BitwiseAndConfig& config, typename T>
void BitwiseAnd(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
                const LocalTensor<T>& src1, const uint32_t count)
```

## Minimal example design
- Inputs: src0 = `12` (0b1100), src1 = `10` (0b1010) -> expect `dst = 8` (0b1000). Exact integer match.
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
