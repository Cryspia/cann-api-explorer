# Ascend C / Or

- Category: vector compute / vector (integer bitwise op (binary), `dst = OP(src0, src1)` element-wise)
- dtype: int16_t (this example); the header also supports int16_t, uint16_t
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Or`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_binary_intf.h`)
```cpp
void Or(const LocalTensor<T>& dst, const LocalTensor<T>& src0, const LocalTensor<T>& src1, const int32_t& count)
```

## Minimal example design
- src0=`6`, src1=`3` -> expect `dst == 7`, host-side element-wise verify (exact match).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
