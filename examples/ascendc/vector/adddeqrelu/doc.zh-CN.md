# Ascend C · AddDeqRelu（矢量，带 dequant 的融合转换）

> English: [doc.md](doc.md)

- 类别：矢量 / 融合 add + dequant + ReLU
- 覆盖 API：`AddDeqRelu`
- include：`kernel_operator.h`
- 来源：CANN 9.1.0 Ascend C API Reference / AddDeqRelu

## 功能
`AddDeqRelu` 融合逐元素加法、dequant 与 ReLU：
`dst = relu( (src0 + src1) * deqScale )`。

dequant scale 是经 `SetDeqScale(half)` 设置的**标量 `g_deqValue`**。3510 实现计算
`max( float(src0+src1) * (1/131072) * g_deqValue * 131072, 0 )`，其中
`1/131072` 与 `131072` 相消，得到 `max((src0+src1)*deqScale, 0)`。这是最简、确定、可校验的
dequant 路径（per-channel 的 `SetDeqScale(LocalTensor, VdeqInfo)` 形式用 bit 打包的
`g_deqScale`，此处不使用）。

## 实测签名
```cpp
void AddDeqRelu(const LocalTensor<half>& dst, const LocalTensor<int32_t>& src0,
                const LocalTensor<int32_t>& src1, const int32_t& count);
// （另有泛型 <T,U> 重载）
```
- src0/src1 为 `int32_t`；dst 为 `half`。`count` 为 `int32_t`。
- 调用 op 之前必须先 `SetDeqScale(half scale)` 设置标量 dequant 值。

## 最简示例设计
- `N = 256`，`deqScale = 2.0`。src0/src1（int32）交替：
  偶数 `i`：`(1 + 2) * 2 = 6`；奇数 `i`：`(-5 + 1) * 2 = -8` → relu → `0`。
- host 解码 half 并校验 `6` / `0`。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
