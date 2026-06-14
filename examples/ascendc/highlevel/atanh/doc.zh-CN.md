# Ascend C / Atanh

- 分类：矢量计算 / highlevel（高阶，`dst = OP(src)` 逐元素（adv_api，简单 count 模式））
- dtype：float（本例）；头文件也支持 half, float
- 来源：见 CANN 9.1.0 Ascend C API 参考

## 功能
AscendC 矢量计算接口 `Atanh`，对 LocalTensor 逐元素计算（反双曲正切）。

## 函数原型（count 模式，取自 toolkit 头文件 `atanh.h`）
```cpp
void Atanh(const LocalTensor<T>& dstTensor, const LocalTensor<T>& srcTensor, const uint32_t calCount)
```

## 最简 example 设计
- src 全填 `0.0` → 期望 `dst ~ 0.0`，host 侧逐元素校验（tol 5e-3）。
- 总长 8*2048，8 核，double buffer；构建 SOC `Ascend950PR_9599`，仿真 `Ascend950`。
- 见同目录 `kernel.cpp` / `main.cpp`；运行结果见 `RESULT.md`。
