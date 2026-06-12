# Ascend C · Matmul（Cube，带 TCubeTiling）

- 分类：高阶 cube / 矩阵乘
- 覆盖 API：`matmul::Matmul` 对象、`REGIST_MATMUL_OBJ`、`SetTensorA/SetTensorB`、`IterateAll`；host `matmul_tiling::MatmulApiTiling`
- include：kernel `lib/matmul_intf.h`；host `tiling/matrix/matmul_tiling.h`
- 原文：CANN 9.1.0 Ascend C API 参考 / Matmul 高阶 API

## 功能
`C[M,N] = A[M,K] @ B[K,N]`，A/B = half，C = float。Cube 单元做 MAD。

## 关键：host 算 TCubeTiling → GM 传入 → kernel REGIST
Matmul 是 Cube 算子，tiling 字段多（50+）且依赖 L1/L0/UB 布局，**必须 host 侧计算**：
```cpp
// host (main.cpp)
matmul_tiling::MatmulApiTiling t;
t.SetAType(GM, ND, DT_FLOAT16); t.SetBType(GM, ND, DT_FLOAT16);
t.SetCType(GM, ND, DT_FLOAT);   t.SetBiasType(GM, ND, DT_FLOAT);
t.SetShape(M,N,K); t.SetOrgShape(M,N,K); t.SetBias(false); t.SetBufferSpace(-1,-1,-1);
AscendC::tiling::TCubeTiling tiling;   // ★ 必须用 kernel 同款结构（见下"坑"）
t.GetTiling(tiling);                   // 拷到 device，作为 kernel 第 5 参
```
```cpp
// kernel (kernel.cpp)
using AType = MatmulType<TPosition::GM, CubeFormat::ND, half>;  // B/C/Bias 类同
TCubeTiling tiling;  /* 从 GM 逐 uint32 拷入 */
Matmul<AType,BType,CType,BiasType> mm;
REGIST_MATMUL_OBJ(&pipe, workspace /*自带 GM workspace*/, mm, &tiling);
mm.SetTensorA(aG); mm.SetTensorB(bG); mm.IterateAll(cG); mm.End();
```

## 踩坑记录（重要）
host 端 **必须用 `GetTiling(AscendC::tiling::TCubeTiling&)`**，不要用 `optiling::TCubeTiling`。
后者用宏（TILING_DATA_FIELD_DEF）定义，与 kernel 读的 `AscendC::tiling::TCubeTiling`（普通 struct，50 字段）**内存布局不同**；
混用会让 kernel 读到错位的零值 → 仿真里 `scalar_rem: div by 0!` + `pem_lsu mem_map unrecognize ldst addr`（地址算成 0/乱值）死循环。改成同款结构后即通。

## 最简 example 设计
- `M=N=K=64`，A/B = half 全 `1.0`(0x3C00) → C = float 全 `K=64`。
- host 校验 C==64（errors=0）；构建链接 `libtiling_api.a` + `libplatform.so`。
- 见 `kernel.cpp` / `main.cpp` / `CMakeLists.txt`，结果见 `RESULT.md`。指令执行数 5245。
