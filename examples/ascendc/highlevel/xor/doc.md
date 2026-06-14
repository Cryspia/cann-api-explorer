# Ascend C / BitwiseXor

- Category: vector compute / highlevel (high-level math, `dst = src0 ^ src1` element-wise bitwise XOR (adv_api, simple count mode))
- dtype: int32 (this example); integer types
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC high-level math interface `BitwiseXor`, computes the element-wise bitwise exclusive-or of two integer LocalTensors: `dst[i] = src0[i] ^ src1[i]`.

## Function prototype (count mode, taken from toolkit header `bitwise_xor.h`)
```cpp
template <const BitwiseXorConfig& config = DEFAULT_BITWISE_XOR_CONFIG, typename T>
void BitwiseXor(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
                const LocalTensor<T>& src1, const uint32_t count);
```

## Minimal example design
- src0 = `6`, src1 = `3` (int32) -> expect `dst = 6 ^ 3 = 5`; integer-exact host-side verify.
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
