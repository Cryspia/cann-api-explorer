# Ascend C / Add

- Category: vector compute / vector (binary, `dst = OP(src0, src1)` element-wise)
- dtype: float (this example); the header also supports half, float
- Source: <https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910beta1/API/ascendcopapi/atlasascendc_api_07_0021.html>

## Functionality
AscendC vector compute interface `Add`, computes element-wise over a LocalTensor.

## Function prototype (count mode, taken from toolkit header `kernel_operator_vec_binary_intf.h`)
```cpp
void Add(const LocalTensor<T>& dst, const LocalTensor<T>& src0, const LocalTensor<T>& src1, const int32_t& count)
```

## Minimal example design
- src0=`1.0`, src1=`2.0` -> expect `dst ~ 3.0`, host-side element-wise verify (tol 1e-3).
- Total length 8*2048, 8 cores, double buffer; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
