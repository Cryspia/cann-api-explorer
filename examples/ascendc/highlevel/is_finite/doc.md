# Ascend C / IsFinite

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: supported (dst, src) combos: (bool/half, half), (bool/float, float), (bool/bfloat16, bfloat16). This example: src `float` -> dst `bool`.
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC adv_api interface `IsFinite`: element-wise finiteness test, `dst = isfinite(src)`, computed over a LocalTensor element-wise. The result is written as `bool` (1 byte, 0/1).

## Function prototype (count mode, taken from toolkit header `is_finite.h`)
```cpp
void IsFinite(const LocalTensor<U>& dst, const LocalTensor<T>& src, uint32_t calCount)
```

## Minimal example design
- Inputs: src = `1.0` -> expect `dst = 1` (true). Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
