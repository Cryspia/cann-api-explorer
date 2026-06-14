# Ascend C · SinCos

- 分类：矢量计算 / highlevel（高阶 math，双输出 `dstSin = sin(src)`、`dstCos = cos(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 高阶 math 接口 `SinCos`，一次调用同时计算输入 LocalTensor 的正弦与余弦，分别写入两个输出 LocalTensor。

## 函数原型（count 模式，摘自 toolkit 头 `sincos.h`）
```cpp
template <const SinCosConfig& config = DEFAULT_SINCOS_CONFIG, typename T>
void SinCos(const LocalTensor<T>& dst0, const LocalTensor<T>& dst1,
            const LocalTensor<T>& src, const uint32_t count);
```
- `dst0` 写入 `sin(src)`，`dst1` 写入 `cos(src)`。

## 最简 example 设计
- src 全填 `0.0` → 期望 `dstSin ≈ 0.0`、`dstCos ≈ 1.0`；host 侧对两个输出逐元素校验（tol 5e-3）。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
