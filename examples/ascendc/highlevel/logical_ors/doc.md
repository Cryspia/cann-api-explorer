# Ascend C / LogicalOrs

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: dst is `bool` (header requires bool output); src0 is a tensor, src1 is a **scalar**. This example: src0 `float`, scalar `float` -> dst `bool`.
- Relation: tensor-scalar counterpart of the tensor-tensor logical ops (see `../logical_and/`). Same elementwise logical-OR semantics, different API entry (the second operand is a scalar broadcast over all elements).
- Source: see the CANN 9.1.0 Ascend C API reference (`adv_api/math/logical_ors.h`)

## Functionality
AscendC adv_api interface `LogicalOrs`: element-wise logical OR of a tensor with a scalar, `dst = (src0 != 0) || (scalar != 0)`. The result is written as `bool` (1 byte, 0/1). With `scalar != 0` the result is always true; with `scalar == 0` it reduces to the truthiness of `src0`.

## Function prototype (count mode, taken from toolkit header `logical_ors.h`, __NPU_ARCH__==3510)
```cpp
template <const LogicalOrsConfig& config, typename T, typename U, typename S>
void LogicalOrs(const LocalTensor<T>& dst, const U& src0, const S& src1, const uint32_t count)
// T = dst dtype (bool), U = src0 tensor, S = src1 scalar
```

## Minimal example design
- Inputs: src0 = `0.0` (false), scalar = `1` (true) -> expect `dst = 1` (true). Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
