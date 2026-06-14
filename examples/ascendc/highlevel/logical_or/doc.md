# Ascend C / LogicalOr

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: dst is `bool`; src supports the integer/float family. This example: src `float` -> dst `bool`.
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC adv_api interface `LogicalOr`: element-wise logical OR, `dst = (src0 != 0) || (src1 != 0)`, computed over a LocalTensor element-wise. The result is written as `bool` (1 byte, 0/1).

## Function prototype (count mode, taken from toolkit header `logical_or.h`)
```cpp
void LogicalOr(const LocalTensor<T>& dst, const LocalTensor<U>& src0, const LocalTensor<U>& src1, const uint32_t count)
```

## Minimal example design
- Inputs: src0 = `1.0`, src1 = `0.0` -> expect `dst = 1` (true). Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
