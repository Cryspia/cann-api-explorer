# Ascend C · SelectWithBytesMask（高阶）

- 分类：高阶 adv_api / select
- 覆盖 API：`AscendC::Select`（tensor-scalar 重载，字节掩码）
- include：`adv_api/select/selectwithbytesmask.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / SelectWithBytesMask

## 实测签名（`__NPU_ARCH__==3510`）
```cpp
template <typename T, typename U, bool isReuseMask = true>
void Select(const LocalTensor<T>& dst, const LocalTensor<T>& src0, T src1,
            const LocalTensor<U>& mask, const LocalTensor<uint8_t>& sharedTmpBuffer,
            const SelectWithBytesMaskShapeInfo& info);
```
- `T`：`half` / `float`。`U`（mask）：`bool/uint8/int8/uint16/int16/uint32/int32`。
- `SelectWithBytesMaskShapeInfo{firstAxis, srcLastAxis, maskLastAxis}`，张量按 `[firstAxis, lastAxis]`
  看待。约束：`srcLastAxis*sizeof(T)` 32B 对齐；`maskLastAxis*sizeof(U)` 32B 对齐且为 16 的倍数；
  `maskLastAxis >= srcLastAxis`。
- 语义（本 tensor-scalar 重载）：掩码字节 `== 0` 取 **src0 张量**；`!= 0` 取 **src1 标量**。
  （另一个重载把标量换到另一侧。）

## 最小可校验设计
- 形状 `[16,16]`，`T=half`，`U=uint8`。`src0[i] = i`，`src1` 标量 `= 99`。
- 每行 mask：前 8 列 `0`（保留 src0=索引），后 8 列 `1`（取标量 99）。
- host 逐元素重建期望矩阵。实测：`d[0]=0, d[8]=99, d[15]=99`，errors=0。

详见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
