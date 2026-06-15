# Ascend C · std（已跳过 —— 不是算子）

> English: [doc.md](doc.md)

- 类别：高阶 adv_api / std
- 状态：**skipped** —— `adv_api/std/` 不是统计（标准差）算子。

## 跳过原因
目录 `aarch64-linux/asc/include/adv_api/std/` 里并没有 “Std” / 标准差算子。它是供其他高阶 API
内部使用的 `AscendC::Std` C++ 标准库垫片命名空间。其头文件为：

- `algorithm.h` —— `AscendC::Std::min` / `AscendC::Std::max`
- `cmath.h` —— `AscendC::Std::sqrt` / `AscendC::Std::abs`
- `type_traits.h` —— `enable_if`、`conditional`、`is_convertible`、`is_base_of`……（编译期 traits）
- `tuple.h` —— `AscendC::Std::tuple` 及辅助
- `utility.h` —— `index_sequence` 等辅助

这些都是纯头文件模板工具（没有 kernel、没有张量 I/O、没有数值 golden 输出），因此这里无法构造一个
有可校验结果、可无卡仿真的单元。在本工具包（CANN 9.1.0-beta.1，arch 3510）上，对 `adv_api/`
grep `__aicore__ Std(...)` 入口或任何 “standard deviation” 算子均无结果。

真正被覆盖的统计/归一化高阶算子位于 `adv_api/normalization/` 及相邻目录：见同级单元 `normalize/`
与 `welfordfinalize/`（以及已有的 `groupnorm/`、`layernorm/`、`batchnorm/`、`deepnorm/`、`rmsnorm/`）。
