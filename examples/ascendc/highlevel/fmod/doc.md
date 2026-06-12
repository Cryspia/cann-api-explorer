# Ascend C / Fmod

- Category: vector compute / highlevel (high-level binary (adv_api), `dst = OP(src0, src1)` element-wise)
- dtype: float (this example); the header also supports half, float
- Source: see the CANN 9.1.0 Ascend C API reference

## Functionality
AscendC vector compute interface `Fmod`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `fmod.h`)
```cpp
void Fmod(const LocalTensor<T>& dstTensor, const LocalTensor<T>& src0Tensor, const LocalTensor<T>& src1Tensor, const uint32_t calCount)
```

## Minimal example design
- src0=`7.0`, src1=`3.0` -> expect `dst ~ 1.0`, host-side element-wise verify (tol 5e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
