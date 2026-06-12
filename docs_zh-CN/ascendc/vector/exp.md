> English: [docs/ascendc/vector/exp.md](../../../docs/ascendc/vector/exp.md)

# Ascend C · Exp(按元素取自然指数)

- 分类:矢量计算 / 数学(指数对数)
- 形状:**unary**(`dst = f(src, count)`)
- 原文:<https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910beta1/API/ascendcopapi/atlasascendc_api_07_0024.html>

## 功能
通过矢量计算单元对输入数据的每个元素计算 e 的 x 次方(逐元素自然指数)。

## 函数原型
```cpp
// 1) tensor 前 n 个数据计算
template <typename T, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           const int32_t& count);

// 2) 高维切分 - mask 逐 bit 模式
template <typename T, bool isSetMask = true, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           uint64_t mask[], const uint8_t repeatTime,
                           const UnaryRepeatParams& repeatParams);

// 3) 高维切分 - mask 连续模式
template <typename T, bool isSetMask = true, const ExpConfig& config = DEFAULT_EXP_CONFIG>
__aicore__ inline void Exp(const LocalTensor<T>& dst, const LocalTensor<T>& src,
                           uint64_t mask, const uint8_t repeatTime,
                           const UnaryRepeatParams& repeatParams);
```

## 参数
- `T`:支持 `half`、`float`。
- `dst` / `src`:`LocalTensor` 操作数,起始地址需 **32 字节对齐**。
- `count`:参与计算的元素个数。
- `mask` / `repeatTime` / `repeatParams`:高维切分模式下控制参与元素与地址步长。
- `config`:仅 Atlas 350 支持,配置 Subnormal 计算模式。

## 约束
- 操作数地址需 32 字节对齐。
- 适用通用的"操作数地址重叠约束"。

## 最简 example 设计
- 取原型 1(count 模式)。输入 `src=0.0` → 期望 `dst=1.0`(e^0=1),host 侧校验。
- arity=unary,dtype=half/float,SOC=Ascend950PR_9599。
