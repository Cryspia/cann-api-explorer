# Ascend C / Not

- Category: vector compute / vector (integer bitwise op (unary), `dst = OP(src)` element-wise)
- dtype: int16_t (this example); the header also supports int16_t, uint16_t
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Not`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_unary_intf.h`)
```cpp
void Not(const LocalTensor<T>& dst, const LocalTensor<T>& src, const int32_t& count)
```

## Minimal example design
- src filled entirely with `0` -> expect `dst == -1`, host-side element-wise verify (exact match).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
