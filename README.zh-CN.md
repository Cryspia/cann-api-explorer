# CANN API Explorer

> English: [README.md](README.md)

为 CANN 9.1.0-beta.1 的 API 逐个编写**最简 example**，用 **cannsim 无卡仿真**（CAModel）实跑并记录性能报告。

## 环境搭建

**系统要求**：仅支持 Linux **aarch64（arm64v8+）** —— CANN `.run` 包与 miniforge 均硬编码为 `linux-aarch64`。自动安装系统依赖库（`libnuma1`/`libgomp1`/`libssl-dev`）假设 **Debian/Ubuntu 系（apt）**发行版；非 apt 系发行版需手动安装这些库。

环境由本目录的 [`install.sh`](install.sh) 搭建：把 CANN Toolkit + cannsim CLI 装进独立 conda env `cannsim`（仅 toolkit，**无 NPU 驱动** —— 纯无卡仿真）：

- `./install.sh` —— 完整安装：下载 CANN `.run` → miniforge3 → 创建 conda env → Toolkit + cannsim wheel → 冒烟测试。
- `./install.sh test` —— 仅测试：cannsim CLI / camodel 检查 + **`vector/add` 单元的真实算子仿真**（经 harness 编译 AscendC 核函数并在 CAModel 上跑通）。
- `./install.sh test-all` —— 测试 + **全量重跑所有单元**（`run_all.sh`）。
- `./install.sh uninstall` —— 删除 conda env（保留 miniforge 与已下载的 `.run`）。

环境变量开关：`ENV_NAME`（默认 `cannsim`）、`PY_VER`、`MINIFORGE_DIR`、`SKIP_REAL_SIM=1`（跳过真实算子仿真）、`RUN_ALL=1`（`test` 时跑全量）。真实算子仿真复用 `harness/run_one.sh examples/ascendc/vector/add`，该脚本自包含（自动 source conda + set_env，构建、record、校验）。

## 当前状态：**84/84 仿真通过** ✅（含 23 个高阶数学算子 + 带 Tiling 算子 SoftMax/LogSoftmax/RmsNorm/LayerNorm/GroupNorm/DeepNorm/BatchNorm/Matmul/TopK/Broadcast + Transpose/Sort/Pad + 数据重排 Duplicate/Gather/Scatter/CreateVecIndex/Brcb + 计算 Axpy/MulCast/Compare/GatherMask + 原子 AtomicAdd + 标量 ScalarCast + 多核同步 SyncAll/IBSet/IBWait + half 变体 AddHalf/CastF2H）

总表（指令数/耗时/报告链接）：[`reports/INDEX.md`](reports/INDEX.md)
性能分析（指令数/耗时横向对比 + 洞察）：[`reports/PERF.zh-CN.md`](reports/PERF.zh-CN.md)
API 文档索引：[`docs_zh-CN/INDEX.md`](docs_zh-CN/INDEX.md)

覆盖的 arity 家族（9 类）：
- **binary** `(dst,src0,src1,count)`：Add/Sub/Mul/Div/Max/Min
- **scalar** `(dst,src,scalar,count)`：Adds/Muls/Subs/Divs/Maxs/Mins
- **unary** `(dst,src,count)`：Exp/Ln/Abs/Sqrt/Rsqrt/Reciprocal/Relu/Neg
- **cast** `(dst,src,roundMode,count)`：Cast(float→int32)
- **reduce** `(dst,src,tmp,count)`：ReduceSum/ReduceMax/ReduceMin
- **activation / math**（adv_api 简单 count 模式）：Sigmoid/Gelu/Silu/Swish + 一元 Sin/Cos/Tan/Tanh/Sinh/Cosh/Asin/Acos/Atan/Erf/Erfc/Floor/Ceil/Round/Rint/Trunc/Sign/Frac + 二元 Power/Fmod/Hypot
- **highlevel + 手填 Tiling**：RmsNorm / LayerNorm / GroupNorm / DeepNorm / BatchNorm / Pad（固定形状下 kernel 内手填少数字段）
- **highlevel + device Tiling**：SoftMax（`SoftMaxTilingFunc`）、LogSoftmax（复用 SoftMaxTilingFunc 填同构 tiling，实测用 log10）、Broadcast（`GetBroadcastTilingInfo`）（kernel 内算 tiling）
- **多核同步**：SyncAll（核间全屏障）、IBSet/IBWait（跨核一对一 flag，链式依赖）（blockDim=8，DataCopy 做 GM 通信）
- **highlevel + host Tiling**：TopK（host `TopKTilingFunc` 算 → GM 传入）
- **highlevel 免 Tiling**：Sort（count 模式，升序）、Transpose（vtranspose b16，16×16）
- **cube + host Tiling**：Matmul（host `matmul_tiling` 算 TCubeTiling → GM 传入 → Cube）
- **manual**（手写）：Select(+CompareScalar)、DataCopy

## 结构

```
manifest.yaml              # 真相源：每个 API 的 arity/dtype/输入/期望/状态
harness/
  templates/*.in           # CMakeLists + 各 arity 的 kernel/host 模板
  gen.py                   # manifest → examples/<lib>/<cat>/<api>/{kernel,main,CMakeLists,meta,doc}
  run_one.sh <dir>         # set_env → build → cannsim record -g → RESULT.md
  run_all.sh [--gen] [..]  # 批量构建+仿真
  aggregate.py             # 汇总 → reports/INDEX.md
examples/<lib>/<cat>/<api>/  # 每个 API 一个单元（doc.md/kernel.cpp/main.cpp/CMakeLists.txt/meta.json/RESULT.md/report/）
docs_zh-CN/INDEX.md        # 人读的 API 索引（英文版在 docs/）
```

每个 example 单元统一：float dtype、8 核、double buffer、构建 SOC `Ascend950PR_9599`、仿真 SOC `Ascend950`。
doc.md 的函数原型由 `gen.py` **从 toolkit 头文件抽取**（权威、离线），非手写猜测。

## 如何新增一个 API

1. 在 `manifest.yaml` 加一条（选 arity、填 inputs/expect；新签名需先看 toolkit 头确认）。
2. `python3 harness/gen.py <Name>` 生成单元。
3. `bash harness/run_one.sh examples/.../<name>` 构建并仿真。
4. `python3 harness/aggregate.py` 刷新总表。

新 arity（签名不同）需在 `harness/templates/` 加模板并在 `gen.py` 的 `ARITY_MAP` 注册。

## 范围与完成状态

- **A 线（可仿真 Ascend C kernel API）= 核心 ✅ 已完成 84/84**：9 个 arity 家族 + 4 类 Tiling 技术（device TilingFunc / 少字段手填 / host tiling 框架 / 免 tiling）+ Cube(Matmul) + 多核同步(SyncAll/IBSet/IBWait) + 数据重排/原子/标量等。本 3510 仿真环境可做成可校验单元的核函数 API 已穷尽；未覆盖项（含 arch 限制的 Conv3D/Conv2D）逐一交代于 [`docs_zh-CN/INDEX.md`](docs_zh-CN/INDEX.md) 「A 线 API 覆盖收尾」。
- **B 线（Runtime/ACL host API）= ✅ 已完成**：发射核函数的 host 脚手架，系统记录于 [`docs_zh-CN/runtime/host_api.md`](docs_zh-CN/runtime/host_api.md)。
- **C 线（GE/HCCL/HIXL/DVPP/ATB/SiP）= 文档说明不覆盖**：逐库「为何不覆盖 + 将来条件」见 [`docs_zh-CN/notcovered.md`](docs_zh-CN/notcovered.md)（需真实驱动/多卡/专用硬件）。
- **待真实环境**：Conv3D/Conv2D（toolkit 仅 m220 实现，需 910B 系环境验证）。
