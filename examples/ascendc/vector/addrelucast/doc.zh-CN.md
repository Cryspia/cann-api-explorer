# Ascend C · AddReluCast（矢量，融合转换）

> English: [doc.md](doc.md)

- 类别：矢量 / 融合 add + ReLU + cast
- 覆盖 API：`AddReluCast`
- include：`kernel_operator.h`
- 来源：CANN 9.1.0 Ascend C API Reference / AddReluCast

## 功能
`AddReluCast` 融合逐元素加法、ReLU 与类型转换：
`dst = cast_to_T( relu(src0 + src1) )`。无需 dequant scale（区别于 `AddDeqRelu`）。

## 实测签名
```cpp
template <typename T, typename U>
void AddReluCast(const LocalTensor<T>& dst, const LocalTensor<U>& src0,
                 const LocalTensor<U>& src1, const uint32_t count);
```
- 本 SoC Level-2（count 模式）dtype 组合：`src(U) = float`、`dst(T) = half`
  （impl 还列出 `<float,int64>`、`<int32,int64>` 组合）。

## 最简示例设计
- `N = 256`。`src0[i]` 交替 `-3 / 2`，`src1 = 1`（float）。
- 偶数 `i`：`relu(-3 + 1) = relu(-2) = 0`；奇数 `i`：`relu(2 + 1) = relu(3) = 3`，cast 到 half。
- host 解码 half 并校验 `0` / `3`。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
