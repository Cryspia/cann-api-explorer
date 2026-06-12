# Ascend C · Not

- 分类：矢量计算 / vector（整型位运算（一元），`dst = OP(src)` 逐元素）
- dtype：int16_t（本例）；头文件另支持 int16_t, uint16_t
- 原文：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 矢量计算接口 `Not`，对 LocalTensor 按元素计算。

## 函数原型（count 模式，摘自 toolkit 头 `kernel_operator_vec_unary_intf.h`）
```cpp
void Not(const LocalTensor<T>& dst, const LocalTensor<T>& src, const int32_t& count)
```

## 最简 example 设计
- src 全填 `0` → 期望 `dst == -1`，host 侧逐元素校验（精确匹配）。
- 总长 8*2048，8 核，double buffer；SOC 构建 `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`，运行结果见 `RESULT.md`。
