# Ascend C · CastDequant（矢量，cast + dequant）

> English: [doc.md](doc.md)

- 类别：矢量 / cast + dequant
- 覆盖 API：`CastDequant`（`CastDeq` 是其废弃别名）
- include：`kernel_operator.h`
- 来源：CANN 9.1.0 Ascend C API Reference / CastDequant

## 功能
`CastDequant` 把整型源数据 cast 并用标量 dequant scale 反量化。
头文件注明 `CastDeq` 已被 `CastDequant` 取代（同一操作，旧名）。

`int32 -> half` 路径下 3510 实现计算
`float(src) * (1/131072) * g_deqValue * 131072`，其中 `131072` 因子相消，得到
`dst = src * g_deqValue`（half）。dequant scale 是经 `SetDeqScale(half)` 设置的**标量 `g_deqValue`**。
`int16 -> int8/uint8` 路径则用 bit 打包的 `g_deqScale`，此处不使用。

## 实测签名
```cpp
template <typename T, typename U, bool isVecDeq = true, bool halfBlock = true>
void CastDequant(const LocalTensor<T>& dst, const LocalTensor<U>& src, const uint32_t count);
// CastDeq 签名相同；头文件提示改用 CastDequant。
```
- 支持的 dtype 组合：`src:int16 -> dst:int8/uint8`，以及 `src:int32 -> dst:half`（此处使用）。

## 最简示例设计
- `N = 256`，`deqScale = 2.0`。src 全 `3`（int32）→ `dst = 3 * 2 = 6`（half）。
- host 解码 half 并校验 `== 6`。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
