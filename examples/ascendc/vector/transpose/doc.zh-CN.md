# Ascend C · Transpose（16×16 块转置）

- 分类：矢量计算 / 数据变换（基础 API）
- 覆盖 API：`Transpose`（硬件 `vtranspose`，b16 16×16）

## 功能
`Transpose(dst, src)`：`dst[i][j] = src[j][i]`，对 16×16 的 b16（half/uint16）块做硬件转置。

## 函数原型
```cpp
template <typename T> __aicore__ inline void Transpose(const LocalTensor<T>& dst, const LocalTensor<T>& src);
```
- 本 arch(3510) 经 `TransposeImpl` → `vtranspose`，仅支持 b16、固定 16×16。
- 更一般形状用带 `TransposeParamsExt` 的重载或 adv_api `ConfusionTranspose`（需 tiling）。

## 最简 example 设计
- 16×16 half，`src[r][c]=r*16+c`（0..255，half 精确）。转置后 `dst[r][c]=c*16+r`。
- host 极简 half 编解码逐元素校验。见 `kernel.cpp`/`main.cpp`，结果 `RESULT.md`。
