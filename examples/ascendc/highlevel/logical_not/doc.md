# Ascend C / LogicalNot

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: dst is `bool`; src supports the integer/float family. This example: src `float` -> dst `bool`.
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC adv_api interface `LogicalNot`: element-wise logical NOT, `dst = !(src != 0)`, computed over a LocalTensor element-wise. The result is written as `bool` (1 byte, 0/1).

## Function prototype (count mode, taken from toolkit header `logical_not.h`)
```cpp
void LogicalNot(const LocalTensor<T>& dst, const LocalTensor<U>& src, const uint32_t count)
```

## Minimal example design
- Inputs: src = `0.0` -> expect `dst = 1` (true); `src = 5` -> `0`. Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
