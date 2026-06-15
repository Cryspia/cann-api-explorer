# Ascend C / LogicalAnds

- Category: vector compute / highlevel (logic / check, output is bool, adv_api simple count mode)
- dtype: dst is `bool` (header requires bool output); src0 is a tensor, src1 is a **scalar**. This example: src0 `float`, scalar `float` -> dst `bool`.
- Relation: this is the **tensor-scalar** counterpart of `LogicalAnd` (tensor-tensor, see `../logical_and/`). Same elementwise logical-AND semantics, different API entry (the second operand is a scalar broadcast over all elements).
- Source: see the CANN 9.1.0 Ascend C API reference (`adv_api/math/logical_ands.h`)

## Functionality
AscendC adv_api interface `LogicalAnds`: element-wise logical AND of a tensor with a scalar, `dst = (src0 != 0) && (scalar != 0)`. The result is written as `bool` (1 byte, 0/1).

## Function prototype (count mode, taken from toolkit header `logical_ands.h`, __NPU_ARCH__==3510)
```cpp
template <const LogicalAndsConfig& config, typename T, typename U, typename S>
void LogicalAnds(const LocalTensor<T>& dst, const U& src0, const S& src1, const uint32_t count)
// T = dst dtype (bool), U = src0 tensor, S = src1 scalar
```

## Minimal example design
- Inputs: src0 = `1.0` (true), scalar = `1` (true) -> expect `dst = 1` (true). Host-side reads the `bool` output as `uint8_t` and verifies 0/1.
- Length 64, single core; SOC build `Ascend950PR_9599`, simulation `Ascend950`.
- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.
