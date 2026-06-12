# Ascend C · Select（+ CompareScalar）

- 分类：矢量计算 / 比较选择（手写组合样例）
- 覆盖 API：`CompareScalar`、`Select`
- 原文：CANN 9.1.0 Ascend C API 参考 / 矢量比较选择

## 功能
- `CompareScalar(mask, src, scalar, cmpMode, count)`：逐元素比较，结果按 bit 写入 uint8 掩码 `mask`。
- `Select(dst, mask, src0, src1, selMode, count)`：按 `mask` 逐元素从 `src0`/`src1` 选择。

## 函数原型（count 模式，摘自 toolkit 头 `kernel_operator_vec_cmpsel_intf.h`）
```cpp
void CompareScalar(const LocalTensor<U>& dst, const LocalTensor<T>& src0,
                   const T src1Scalar, CMPMODE cmpMode, uint32_t count);
void Select(const LocalTensor<T>& dst, const LocalTensor<U>& selMask,
            const LocalTensor<T>& src0, const LocalTensor<T>& src1,
            SELMODE selMode, uint32_t count);
```
- `CMPMODE`：LT/GT/EQ/LE/GE/NE；`SELMODE`：VSEL_TENSOR_TENSOR_MODE 等。
- 掩码 `U=uint8_t`，按 bit 紧凑存储（count 个元素占 count/8 字节）。

## 最简 example 设计
- x 全填 `1.0`，y 全填 `2.0`。
- `mask = CompareScalar(x > 0.0)` → 全真（1>0）。
- `z = Select(mask, x, y, VSEL_TENSOR_TENSOR_MODE)` → 全取 x → 期望 `z = 1.0`。
- 这样只需校验 Select 的 float 输出，避免逐 bit 校验掩码。
- 见同目录 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
