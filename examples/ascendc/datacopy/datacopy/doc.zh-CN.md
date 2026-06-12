# Ascend C · DataCopy（数据搬运）

- 分类：数据搬运（手写 identity 样例）
- 覆盖 API：`DataCopy`（GM↔Local、Local↔Local）
- 原文：CANN 9.1.0 Ascend C API 参考 / 数据搬运

## 功能
`DataCopy(dst, src, count)`：在 Global Memory 与 Local Memory（或 Local↔Local）之间搬运 `count` 个元素。
是所有矢量/标量 kernel 的 CopyIn/CopyOut 基础（本项目每个 example 都在用）。

## 函数原型（常用 count 形式）
```cpp
template <typename T>
__aicore__ inline void DataCopy(const LocalTensor<T>& dstLocal,
                                const GlobalTensor<T>& srcGlobal, const uint32_t count);
// 反向：DataCopy(GlobalTensor dst, LocalTensor src, count)；以及 Local↔Local
```
- 起始地址需 32 字节对齐；`count` 需满足该 dtype 的对齐粒度。

## 最简 example 设计
- x 全填 `7.0`。kernel 不做任何计算，只 `DataCopy`：GM→Local→Local→GM。
- 期望 `z == x == 7.0`，host 逐元素校验。
- 见同目录 `kernel.cpp` / `main.cpp`，结果见 `RESULT.md`。
