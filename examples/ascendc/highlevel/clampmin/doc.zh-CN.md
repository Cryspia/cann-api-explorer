# Ascend C · ClampMin

- 分类：矢量计算 / highlevel（高阶 math，`dst = max(src, scalar)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件另支持 half, float
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 高阶 math 接口 `ClampMin`，将所有小于 `scalar` 的元素替换为 `scalar`（即 `dst[i] = max(src[i], scalar)`）。

## 函数原型（count 模式，摘自 toolkit 头 `clamp.h`）
```cpp
template <typename T, bool isReuseSource = false>
void ClampMin(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor,
              const T scalar, const uint32_t calCount);
```

## 最简 example 设计
- src 全填 `1.0`，`scalar = 3.0` → 期望 `dst ≈ 3.0`；host 侧逐元素校验（tol 5e-3）。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
