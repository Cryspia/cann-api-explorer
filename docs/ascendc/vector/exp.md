> 中文版: [docs_zh-CN/ascendc/vector/exp.md](../../../docs_zh-CN/ascendc/vector/exp.md)

# Ascend C · Exp (element-wise natural exponential)

- Category: vector compute / math (exponential & logarithm)
- Shape: **unary** (`dst = f(src, count)`)
- Source: <https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910beta1/API/ascendcopapi/atlasascendc_api_07_0024.html>

## Functionality
Uses the vector compute unit to compute e to the power of x for each element of the input data (element-wise natural exponential).

## Function prototype
```cpp
// 1) compute the first n elements of the tensor
template <typename T, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           const int32_t& count);

// 2) high-dimensional tiling - per-bit mask mode
template <typename T, bool isSetMask = true, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           uint64_t mask[], const uint8_t repeatTime,
                           const UnaryRepeatParams& repeatParams);

// 3) high-dimensional tiling - continuous mask mode
template <typename T, bool isSetMask = true, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           uint64_t mask, const uint8_t repeatTime,
                           const UnaryRepeatParams& repeatParams);
```

## Parameters
- `T`: supports `half`, `float`.
- `dst` / `src`: `LocalTensor` operands, the start address must be **32-byte aligned**.
- `count`: the number of elements participating in the computation.
- `mask` / `repeatTime` / `repeatParams`: in high-dimensional tiling mode, control the participating elements and address strides.
- `config`: only supported on Atlas 350, configures the Subnormal compute mode.

## Constraints
- Operand addresses must be 32-byte aligned.
- The general "operand address overlap constraint" applies.

## Minimal example design
- Take prototype 1 (count mode). Input `src=0.0` → expected `dst=1.0` (e^0=1), verified on the host side.
- arity=unary, dtype=half/float, SOC=Ascend950PR_9599.
