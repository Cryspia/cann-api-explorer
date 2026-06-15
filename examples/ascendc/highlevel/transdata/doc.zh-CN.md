# Ascend C · TransData（高阶）

- 分类：高阶 adv_api / transpose（格式转换）
- 覆盖 API：`AscendC::TransData`（ND <-> 分形 5D 格式转换）
- include：`adv_api/transpose/transdata.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / TransData

## 实测签名（`__NPU_ARCH__==3510`）
```cpp
template <const TransDataConfig& config, typename T, typename U, typename S>
void TransData(const LocalTensor<T>& dst, const LocalTensor<T>& src,
               const LocalTensor<uint8_t>& sharedTmpBuffer,
               const TransDataParams<U, S>& params);   // params = {srcLayout, dstLayout}
```
- `config` 是编译期 `TransDataConfig{srcFormat, dstFormat}`。3510 上支持的组合为
  `NCDHW <-> NDC1HWC0` 与 `NCDHW <-> FRACTAL_Z_3D`（5D 格式转换）。
- `T`：`half` / `bfloat16` / `uint16` / `int16`。
- `params.srcLayout` / `params.dstLayout` 是 CuTe 风格 `Layout`，用
  `MakeLayout(MakeShape(...), MakeStride(...))` 构造。NCDHW 形状 5 维、NDC1HWC0 6 维、FRACTAL_Z_3D 7 维
  （`static_assert` 校验）。3510 的重排只读 NCDHW 侧 `(n,c,d,h,w)`。
- 转换基于 `TransDataTo5HD` 16x16 分块转置，需要 `uint8_t` 的 `sharedTmpBuffer`（本例 16KB）。

## host 编译 pass 注意
`transdata.h` 把 `TransDataConfig` 与 `TransData` 放在 `__NPU_ARCH__` 守卫内，而 host-only kernel 编译
pass 下 `__NPU_ARCH__` **未定义**。因此 device 代码（config + `TransData` 调用）用
`#if defined(__NPU_ARCH__)` 包起来，使 host pass 仍能产出 launch stub。

## 最小可校验设计（往返）
不去手工解码分形字节排布，本单元校验一个恒等 **往返**：`NCDHW -> NDC1HWC0 -> NCDHW`。
形状 NCDHW `[1,16,1,4,4]`（`C=16=c0` 无通道 padding；`H*W=16` 无空间 padding），`T=half`，
`src[i] = i+1`。往返须复现全部 256 个元素。实测：`d[0]=1, d[128]=129, d[255]=256`，errors=0。

详见 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
