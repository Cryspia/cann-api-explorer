# Ascend C · BitwiseXor

- 分类：矢量计算 / highlevel（高阶 math，`dst = src0 ^ src1` 逐元素按位异或（adv_api，简单 count 模式））
- dtype：int32（本例）；整型
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 高阶 math 接口 `BitwiseXor`，对两个整型 LocalTensor 逐元素计算按位异或：`dst[i] = src0[i] ^ src1[i]`。

## 函数原型（count 模式，摘自 toolkit 头 `bitwise_xor.h`）
```cpp
template <const BitwiseXorConfig& config = DEFAULT_BITWISE_XOR_CONFIG, typename T>
void BitwiseXor(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
                const LocalTensor<T>& src1, const uint32_t count);
```

## 最简 example 设计
- src0 = `6`，src1 = `3`（int32）→ 期望 `dst = 6 ^ 3 = 5`；host 侧整型精确校验。
- 长度 64，单核；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
