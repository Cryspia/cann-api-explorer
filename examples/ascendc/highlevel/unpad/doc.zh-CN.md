# Ascend C · UnPad（高阶，pad）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / pad
- 覆盖 API：`UnPad`（`Pad` 的逆操作）
- include：`lib/pad/pad.h`
- 来源：CANN 9.1.0 Ascend C API Reference / UnPad

## 功能
`UnPad` 是 `Pad` 的逆操作：从每行右侧去掉 padding 列。`Pad` 在每行追加 `rightPad` 个元素，
`UnPad` 则在每行去掉 `rightPad` 个元素，得到 `srcWidth - rightPad` 个有效元素。

## 实测签名
```cpp
template <typename T>
void UnPad(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor,
           UnPadParams& unPadParams, LocalTensor<uint8_t>& sharedTmpBuffer, UnPadTiling& tiling);
// 另有一个免 tmp 的重载，内部自动取栈缓冲。
```
- `UnPadParams(leftPad, rightPad)` — 均为 `uint16_t`。3510 实现只读 `rightPad`；
  本架构忽略 `leftPad`（保留只为与 `Pad` 对称）。
- `UnPadTiling` — 3510 实现读取 `srcHeight`、`srcWidth`，每行从 src 拷贝
  `srcWidth - rightPad` 个有效元素到 dst。
- `T` 支持 `int16_t/uint16_t/half/int32_t/uint32_t/float`。

## 最简示例设计
- 单行，`srcWidth = 16`（16*4 = 64B，32B 对齐），`rightPad = 4`。
- `src = [1..16]` → `dst[0..11] = [1..12]`，最后 4 个 padding 列被去掉。
- 与已有 `pad` 单元对称（`src=[1..16]`、`rightPad=4`、追加 pad 值）。
- host 精确校验 12 个有效元素。见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
