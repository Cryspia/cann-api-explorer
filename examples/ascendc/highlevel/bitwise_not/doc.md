# Ascend C / BitwiseNot

- Category: vector compute / highlevel (integer bitwise, adv_api simple count mode)
- dtype: integer; src and dst share the same type `T`. Level-2 integer dtypes on this SoC: `int16`/`uint16`/`int64`/`uint64` (`int32` is NOT supported), so this unit uses `int16`.
- Relation: this is the **adv_api (`lib/math`) count-mode** entry for per-element bitwise NOT (one's complement). The identical operation is also exposed by the lower-level vector intrinsic `AscendC::Not` (see `../../vector/not/`); same semantics, different API layer.
- Source: see the CANN 9.1.0 Ascend C API reference (`adv_api/math/bitwise_not.h`)

## Functionality
`BitwiseNot`: element-wise bitwise NOT of an integer tensor, `dst[i] = ~src[i]`. For a signed two's-complement integer this is `~x = -(x+1)`, so e.g. `~5 = -6` and `~0 = -1`.

## Function prototype (count mode, taken from toolkit header `bitwise_not.h`, __NPU_ARCH__==3510)
```cpp
template <const BitwiseNotConfig& config, typename T>
void BitwiseNot(const LocalTensor<T>& dst, const LocalTensor<T>& src, const uint32_t count)
```

## Minimal example design
- Inputs: src = `5` -> expect `dst = -6` (signed int16 two's complement). Exact integer match.
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
