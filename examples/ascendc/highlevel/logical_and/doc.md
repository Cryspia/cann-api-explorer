# Ascend C / LogicalAnd

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: dst is `bool` (header requires bool output); src `bool/uint8_t/int8_t/half/bfloat16_t/uint16_t/int16_t/float/...`. This example: src `float` -> dst `bool`.
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC adv_api interface `LogicalAnd`: element-wise logical AND, `dst = (src0 != 0) && (src1 != 0)`, computed over a LocalTensor element-wise. The result is written as `bool` (1 byte, 0/1).

## Function prototype (count mode, taken from toolkit header `logical_and.h`)
```cpp
void LogicalAnd(const LocalTensor<T>& dst, const LocalTensor<U>& src0, const LocalTensor<U>& src1, const uint32_t count)
```

## Minimal example design
- Inputs: src0 = `1.0`, src1 = `0.0` -> expect `dst = 0` (false). Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
