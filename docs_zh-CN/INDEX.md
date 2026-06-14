> English: [docs/INDEX.md](../docs/INDEX.md)

# CANN 9.1.0-beta.1 API 本地文档索引

文档源:<https://www.hiascend.com/document/detail/zh/CANNCommunityEdition/910beta1/index/index.html>

状态图例:`📝`=已建 doc `🧪`=有 example `✅`=仿真跑通 `⬜`=待办 `🚫`=不覆盖(需硬件/多卡)

> 总表(指令数/耗时/报告链接)见 [`../reports/INDEX.md`](../reports/INDEX.md),由 `harness/aggregate.py` 自动生成。
> 每个 API 的功能说明 + 真实原型(摘自 toolkit 头) + example 设计见各单元目录 `examples/.../<api>/doc.md`。

## A 线 · Ascend C 核函数 API(可 cannsim 仿真)

### 矢量计算 vector/ —— ✅ 56/56 仿真通过
binary=`(dst,src0,src1,count)`,unary=`(dst,src,count)`,scalar=`(dst,src,scalar,count)`;
本批 example 统一用 float、8 核、double buffer、SOC 构建 `Ascend950PR_9599` / 仿真 `Ascend950`。

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| Add | binary | [doc](../examples/ascendc/vector/add/doc.zh-CN.md) | ✅ |
| Sub | binary | [doc](../examples/ascendc/vector/sub/doc.zh-CN.md) | ✅ |
| Mul | binary | [doc](../examples/ascendc/vector/mul/doc.zh-CN.md) | ✅ |
| Div | binary | [doc](../examples/ascendc/vector/div/doc.zh-CN.md) | ✅ |
| Max | binary | [doc](../examples/ascendc/vector/max/doc.zh-CN.md) | ✅ |
| Min | binary | [doc](../examples/ascendc/vector/min/doc.zh-CN.md) | ✅ |
| Adds | scalar | [doc](../examples/ascendc/vector/adds/doc.zh-CN.md) | ✅ |
| Muls | scalar | [doc](../examples/ascendc/vector/muls/doc.zh-CN.md) | ✅ |
| Subs | scalar | [doc](../examples/ascendc/vector/subs/doc.zh-CN.md) | ✅ |
| Divs | scalar | [doc](../examples/ascendc/vector/divs/doc.zh-CN.md) | ✅ |
| Maxs | scalar | [doc](../examples/ascendc/vector/maxs/doc.zh-CN.md) | ✅ |
| Mins | scalar | [doc](../examples/ascendc/vector/mins/doc.zh-CN.md) | ✅ |
| Exp | unary | [doc](../examples/ascendc/vector/exp/doc.zh-CN.md) | ✅ |
| Ln | unary | [doc](../examples/ascendc/vector/ln/doc.zh-CN.md) | ✅ |
| Abs | unary | [doc](../examples/ascendc/vector/abs/doc.zh-CN.md) | ✅ |
| Sqrt | unary | [doc](../examples/ascendc/vector/sqrt/doc.zh-CN.md) | ✅ |
| Rsqrt | unary | [doc](../examples/ascendc/vector/rsqrt/doc.zh-CN.md) | ✅ |
| Reciprocal | unary | [doc](../examples/ascendc/vector/reciprocal/doc.zh-CN.md) | ✅ |
| Relu | unary | [doc](../examples/ascendc/vector/relu/doc.zh-CN.md) | ✅ |
| Neg | unary | [doc](../examples/ascendc/vector/neg/doc.zh-CN.md) | ✅ |
| Cast | cast | [doc](../examples/ascendc/vector/cast/doc.zh-CN.md) | ✅ |
| ReduceSum | reduce | [doc](../examples/ascendc/vector/reducesum/doc.zh-CN.md) | ✅ |
| ReduceMax | reduce | [doc](../examples/ascendc/vector/reducemax/doc.zh-CN.md) | ✅ |
| ReduceMin | reduce | [doc](../examples/ascendc/vector/reducemin/doc.zh-CN.md) | ✅ |
| And | bitbin(int16) | [doc](../examples/ascendc/vector/and/doc.zh-CN.md) | ✅ |
| Or | bitbin(int16) | [doc](../examples/ascendc/vector/or/doc.zh-CN.md) | ✅ |
| Not | bitun(int16) | [doc](../examples/ascendc/vector/not/doc.zh-CN.md) | ✅ |
| Select(+CompareScalar) | manual/mask | [doc](../examples/ascendc/vector/select/doc.zh-CN.md) | ✅ |
| Transpose | manual/vtranspose(b16) | [doc](../examples/ascendc/vector/transpose/doc.zh-CN.md) | ✅ |
| Duplicate | 填充(dst=scalar) | [doc](../examples/ascendc/vector/duplicate/doc.zh-CN.md) | ✅ |
| CreateVecIndex | 索引生成(dst[i]=i) | [doc](../examples/ascendc/vector/createvecindex/doc.zh-CN.md) | ✅ |
| Gather | 收集(字节 offset) | [doc](../examples/ascendc/vector/gather/doc.zh-CN.md) | ✅ |
| Scatter | 散射(字节 offset) | [doc](../examples/ascendc/vector/scatter/doc.zh-CN.md) | ✅ |
| Brcb | block broadcast | [doc](../examples/ascendc/vector/brcb/doc.zh-CN.md) | ✅ |
| Axpy | 融合乘加 y=a*x+y | [doc](../examples/ascendc/vector/axpy/doc.zh-CN.md) | ✅ |
| MulCast | 乘+类型转换(int64→int32) | [doc](../examples/ascendc/vector/mulcast/doc.zh-CN.md) | ✅ |
| Compare | 两向量比较出 mask(+Select) | [doc](../examples/ascendc/vector/compare/doc.zh-CN.md) | ✅ |
| GatherMask | 流压缩(pattern=1 选偶数) | [doc](../examples/ascendc/vector/gathermask/doc.zh-CN.md) | ✅ |
| AtomicAdd | GM 原子累加(8核→8) | [doc](../examples/ascendc/vector/atomicadd/doc.zh-CN.md) | ✅ |
| AddHalf | Add 的 half 变体 | [doc](../examples/ascendc/vector/add_half/doc.zh-CN.md) | ✅ |
| CastF2H | Cast float→half | [doc](../examples/ascendc/vector/castf2h/doc.zh-CN.md) | ✅ |
| PRelu | 参数化 ReLU，x>0?x:a*x（逐元素斜率） | [doc](../examples/ascendc/vector/prelu/doc.zh-CN.md) | ✅ |
| LeakyRelu | 带泄漏 ReLU，x>0?x:alpha*x（标量斜率） | [doc](../examples/ascendc/vector/leakyrelu/doc.zh-CN.md) | ✅ |
| ShiftLeft | 按位左移(dst=src0<<src1) | [doc](../examples/ascendc/vector/shiftleft/doc.zh-CN.md) | ✅ |
| ShiftRight | 按位右移(dst=src0>>src1) | [doc](../examples/ascendc/vector/shiftright/doc.zh-CN.md) | ✅ |
| Ands | 与标量按位与(dst=src&scalar) | [doc](../examples/ascendc/vector/ands/doc.zh-CN.md) | ✅ |
| Ors | 与标量按位或(dst=src|scalar) | [doc](../examples/ascendc/vector/ors/doc.zh-CN.md) | ✅ |
| AddRelu | 融合 relu(src0+src1) | [doc](../examples/ascendc/vector/addrelu/doc.zh-CN.md) | ✅ |
| SubRelu | 融合 relu(src0-src1) | [doc](../examples/ascendc/vector/subrelu/doc.zh-CN.md) | ✅ |
| MulsCast | 乘标量+类型转换 | [doc](../examples/ascendc/vector/mulscast/doc.zh-CN.md) | ✅ |
| FusedMulAdd | 融合 dst=dst*src0+src1（原地乘加） | [doc](../examples/ascendc/vector/fusedmuladd/doc.zh-CN.md) | ✅ |
| MulAddDst | 融合 dst=dst+src0*src1 | [doc](../examples/ascendc/vector/muladddst/doc.zh-CN.md) | ✅ |
| MulAddRelu | 融合 relu(dst+src0*src1) | [doc](../examples/ascendc/vector/muladdrelu/doc.zh-CN.md) | ✅ |
| AbsSub | 融合 abs(src0-src1) | [doc](../examples/ascendc/vector/abssub/doc.zh-CN.md) | ✅ |
| ExpSub | 融合 exp(src0-src1) | [doc](../examples/ascendc/vector/expsub/doc.zh-CN.md) | ✅ |
| Mull | 整数乘低半部 | [doc](../examples/ascendc/vector/mull/doc.zh-CN.md) | ✅ |

矢量类 **56/56 通过**，覆盖 binary/unary/scalar/cast/reduce/bitbin/bitun + 手写掩码(Compare/CompareScalar+Select) + Transpose + 数据重排(Duplicate/CreateVecIndex/Gather/Scatter/Brcb) + 融合(Axpy/MulCast/FusedMulAdd/MulAddDst/MulAddRelu/AddRelu/SubRelu/AbsSub/ExpSub) + 激活变体(PRelu/LeakyRelu) + 标量/位运算(ShiftLeft/ShiftRight/Ands/Ors/MulsCast/Mull) + 原子(AtomicAdd)。

### 标量计算 scalar/ —— ✅ 1/1 仿真通过

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| ScalarCast | 标量转换(3.7→4, CAST_RINT) | [doc](../examples/ascendc/scalar/scalarcast/doc.zh-CN.md) | ✅ |

### 同步 sync/ —— ✅ 2/2 仿真通过

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| SyncAll | 多核全屏障（blockDim=8，DataCopy 做 GM 核间通信） | [doc](../examples/ascendc/sync/syncall/doc.zh-CN.md) | ✅ |
| IBSet/IBWait | 跨核一对一 flag（链式 0→1→…→7 依赖，比 SyncAll 细粒度） | [doc](../examples/ascendc/sync/ibsetwait/doc.zh-CN.md) | ✅ |

> 标量 / 搬运 / 内存资源类的其余 API（ScalarCast、Copy、TPipe/TQue/TBuf/TBufPool 等）见下文「A 线未覆盖 API 清单」统一记录。

### 高阶 API highlevel/ —— ✅ 57/57 仿真通过（5 激活 + 28 数学 + 6 归一化 + 7 逻辑/比较 + 6 逐元素/工具 + TopK/Sort/Pad/Broadcast + LogSoftmax）

**激活类**（简单 `(dst,src,count)`，免 Tiling，`#include "lib/activation/<op>.h"`）:

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| Sigmoid | activation | [doc](../examples/ascendc/highlevel/sigmoid/doc.zh-CN.md) | ✅ |
| Gelu | activation | [doc](../examples/ascendc/highlevel/gelu/doc.zh-CN.md) | ✅ |
| Silu | activation | [doc](../examples/ascendc/highlevel/silu/doc.zh-CN.md) | ✅ |
| Swish | activation | [doc](../examples/ascendc/highlevel/swish/doc.zh-CN.md) | ✅ |
| GeGLU | 门控 GLU 激活，gelu(a)*b | [doc](../examples/ascendc/highlevel/geglu/doc.zh-CN.md) | ✅ |

**高阶数学类** adv_api/math（免 tmp `(dst,src,count)` / `(dst,src0,src1,count)` 重载,`#include "lib/math/<op>.h"`）—— ✅ 25/25:
- 一元(activation arity, 24)：Sin/Cos/Tan/Tanh/Sinh/Cosh/Asin/Acos/Atan/Asinh/Acosh/Atanh/Erf/Erfc/Floor/Ceil/Round/Rint/Trunc/Sign/Frac/Log/Lgamma/Digamma
- 二元(binary_act arity, 3)：Power(2³=8)/Fmod(7%3=1)/Hypot(3,4=5)
- 双输出(1)：SinCos（单 src → 同时输出 sin 与 cos）
（Asinh=反双曲正弦，Acosh=反双曲余弦，Atanh=反双曲正切，Digamma=ψ(x) 即 Γ 的对数导数；各单元 doc 见 `examples/ascendc/highlevel/<op>/doc.zh-CN.md`）。

**逻辑 / 比较类**（布尔输出逐元素，`#include "lib/math/<op>.h"`）—— ✅ 7/7:

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| LogicalAnd | 逻辑与(bool 输出) | [doc](../examples/ascendc/highlevel/logical_and/doc.zh-CN.md) | ✅ |
| LogicalOr | 逻辑或(bool 输出) | [doc](../examples/ascendc/highlevel/logical_or/doc.zh-CN.md) | ✅ |
| LogicalXor | 逻辑异或(bool 输出) | [doc](../examples/ascendc/highlevel/logical_xor/doc.zh-CN.md) | ✅ |
| LogicalNot | 逻辑非(bool 输出) | [doc](../examples/ascendc/highlevel/logical_not/doc.zh-CN.md) | ✅ |
| IsNan | NaN 判定(bool 输出) | [doc](../examples/ascendc/highlevel/is_nan/doc.zh-CN.md) | ✅ |
| IsInf | Inf 判定(bool 输出) | [doc](../examples/ascendc/highlevel/is_inf/doc.zh-CN.md) | ✅ |
| IsFinite | 有限值判定(bool 输出) | [doc](../examples/ascendc/highlevel/is_finite/doc.zh-CN.md) | ✅ |

**逐元素 / 工具类** —— ✅ 6/6:

| API | 形状 | doc | 状态 |
|-----|------|-----|------|
| Fma | 融合乘加 a*b+c | [doc](../examples/ascendc/highlevel/fma/doc.zh-CN.md) | ✅ |
| Xor | 按位异或 | [doc](../examples/ascendc/highlevel/xor/doc.zh-CN.md) | ✅ |
| ClampMax | 钳位上界(与标量取 min) | [doc](../examples/ascendc/highlevel/clampmax/doc.zh-CN.md) | ✅ |
| ClampMin | 钳位下界(与标量取 max) | [doc](../examples/ascendc/highlevel/clampmin/doc.zh-CN.md) | ✅ |
| Where | 条件选择(mask?a:b) | [doc](../examples/ascendc/highlevel/where/doc.zh-CN.md) | ✅ |
| CumSum | 前缀和(沿轴累加) | [doc](../examples/ascendc/highlevel/cumsum/doc.zh-CN.md) | ✅ |

**带 Tiling 的归一化类**（突破点:两种 kernel 内 tiling 技术,免 host tiling 框架）:

| API | tiling 来源 | doc | 状态 |
|-----|------|-----|------|
| SoftMax | device 端 `SoftMaxTilingFunc` 构造 | [doc](../examples/ascendc/highlevel/softmax/doc.zh-CN.md) | ✅ |
| LogSoftmax | 复用 device `SoftMaxTilingFunc` 填同构 `LogSoftMaxTiling`（实测用 **log10**） | [doc](../examples/ascendc/highlevel/logsoftmax/doc.zh-CN.md) | ✅ |
| RmsNorm | 12 字段 `RmsNormTiling` 单 tile 手填 | [doc](../examples/ascendc/highlevel/rmsnorm/doc.zh-CN.md) | ✅ |
| LayerNorm | 3510 regbase `LayerNorm(Para,SeparateTiling)`,rLength≤64 简单分支手填(k2Rec·k2RRec=1/R) | [doc](../examples/ascendc/highlevel/layernorm/doc.zh-CN.md) | ✅ |
| GroupNorm | `GroupNormTiling` 手填(3510 只用 n/g/d/hw/factor=1/(D·HW)) | [doc](../examples/ascendc/highlevel/groupnorm/doc.zh-CN.md) | ✅ |
| DeepNorm | `DeepNormTiling` 手填 5 字段(mean 系数内部用 hLength) | [doc](../examples/ascendc/highlevel/deepnorm/doc.zh-CN.md) | ✅ |
| BatchNorm | `BatchNormTiling` 手填 3 字段(firstDimValueBack=1/B) | [doc](../examples/ascendc/highlevel/batchnorm/doc.zh-CN.md) | ✅ |

**排序 / 填充 / 广播 类**:

| API | tiling 来源 | doc | 状态 |
|-----|------|-----|------|
| TopK | host `TopKTilingFunc` 算 `TopkTiling` → GM 传入(另链 `libgraph_base.so`) | [doc](../examples/ascendc/highlevel/topk/doc.zh-CN.md) | ✅ |
| Sort | 免 tiling(count 模式,**升序**,idx 跟随) | [doc](../examples/ascendc/highlevel/sort/doc.zh-CN.md) | ✅ |
| Pad | `PadTiling` 手填 3 字段(srcHeight/srcWidth/srcOriWidth) | [doc](../examples/ascendc/highlevel/pad/doc.zh-CN.md) | ✅ |
| Broadcast | device 端 `GetBroadcastTilingInfo` 在核内算 | [doc](../examples/ascendc/highlevel/broadcast/doc.zh-CN.md) | ✅ |

**Cube 矩阵乘** cube/ —— host tiling 框架打通:

| API | tiling 来源 | doc | 状态 |
|-----|------|-----|------|
| Matmul | host `matmul_tiling` 算 TCubeTiling → GM 传入 | [doc](../examples/ascendc/cube/matmul/doc.zh-CN.md) | ✅ |
| Conv3D | host `Conv3dTiling` + kernel Conv3D 对象（接口已实现） | [doc](../examples/ascendc/cube/conv3d/doc.zh-CN.md) | ⚠️ 本 arch(3510) 无实现，仅 m220 |

带 Tiling 的四类技术全部验证:**device `*TilingFunc`**(SoftMax/Broadcast)、**少字段手填**(RmsNorm/GroupNorm/DeepNorm/BatchNorm/Pad)、**host tiling 框架**(Matmul/TopK,含 `libtiling_api.a`+`libplatform.so` 链接 + `AscendC::tiling::*` 同款结构)、**免 tiling**(Sort/Transpose)。

### A 线 API 覆盖收尾（已补做 / 已查明不可用 / 仍未覆盖）

已覆盖 **116** 个单元。下表交代曾在「未覆盖清单」里、本轮逐一处置的结果。

**① 已补做成单元（✅，见上文各段）**
Compare、GatherMask、Axpy、MulCast（矢量计算）；AtomicAdd（原子）；ScalarCast（标量）。
新增融合 / 标量 / 位运算（矢量计算）：PRelu、LeakyRelu、ShiftLeft、ShiftRight、Ands、Ors、AddRelu、SubRelu、MulsCast、FusedMulAdd、MulAddDst、MulAddRelu、AbsSub、ExpSub、Mull。
新增高阶算子：Acosh、Asinh、Atanh、Digamma、SinCos、GeGLU、ClampMax、ClampMin、Fma、Xor、LogicalAnd、LogicalOr、LogicalXor、LogicalNot、IsNan、IsInf、IsFinite、Where、CumSum。

**② 已尝试、查明本环境不可用（🚫，代码/反例留存，meta.json.unsupported 不计入通过数）**
| API | 头 | 查明结论 |
|-----|----|---------|
| Conv3D | conv/conv3d | toolkit 仅 dav_m220 实现，3510 编译报 no member（见 cube 段） |
| Conv2D | conv2d | 同属 Cube basic 指令编排，依赖 V220 系 fixpipe，3510 不适用 |
| VectorPadding | vec_vpadding | count 模式 padMode=0 输出全 0，语义需 mask+repeat 精确控制，难校验 |
| DetermineSync（NotifyNextBlock/WaitPreBlock） | determine_compute_sync | 链式裁剪 Wait/Notify 配对致 CAModel **仿真死锁**（sim_exe 607% CPU 不退出）；范式已由 SyncAll+IBSet/IBWait 覆盖 |

**③ 仍未覆盖（与已覆盖项同质 / 需特殊参数 / 辅助设施，按需补）**
| API | 头 | 不补原因 |
|-----|----|---------|
| 量化转换/融合（CastDequant/CastDeq/AddReluCast/AddDeqRelu） | vec_vconv | 量化转换需 deqScale 参数；基础 Cast + 普通 AddRelu 已覆盖非量化语义 |
| FusedMulsCast | vec_vconv | 乘标量+量化转换变体；普通 MulsCast 已覆盖非量化形态 |
| Philox 随机生成 | rand | key/counter 伪随机生成，无可对照的参考数值输出 |
| BilinearInterpolation | vec_bilinearinterpolation | 参数巨多（offset/hRepeat/vRepeat/tmp），无 count 简化模式 |
| Gemm（GetGemmTiling） | gemm | 与 Matmul 同属 Cube+host-tiling，已由 Matmul 代表 |
| Fixpipe | fixpipe | Cube 结果搬运，Matmul 内部隐式使用 |
| Concat / Extract / MrgSort / MrgSort4 | proposal | Sort/TopK 的底层归并原语，排序语义已由 Sort/TopK 覆盖；需 proposal 打包格式 |
| DataCacheCleanAndInvalid / Preload | cache | cache 管理辅助，已在 SyncAll/IB 内部间接用到 dcci |
| DumpTensor / PRINTF | dump_tensor | kernel 内调试输出，非数值算子 |
| ListTensorDesc / SetSysWorkSpacePtr | list_tensor / swap_mem | 描述符 / workspace 指针，辅助基础设施 |

> 说明：**memory 资源类**（TPipe/TQue/TBuf/TBufPool）是基础设施，已内嵌在**每一个** kernel 中使用（InitBuffer/AllocTensor/EnQue…），不单列为算子单元。

### Utils utils/
(平台信息、Tiling 框架、调试接口、C++ 标准库子集)

## B 线 · Runtime / AscendCL host API(host 执行,不产指令仿真报告)

已系统记录：[`runtime/host_api.md`](runtime/host_api.md) —— 盘点自各单元 main.cpp 的全部 host API（13 个 acl* + ACLRT_LAUNCH_KERNEL + platform），含标准调用序列与分组说明（初始化/设备/Stream/内存/核函数发射/平台 tiling）。这些 API 是发射核函数的脚手架，不产 CAModel 报告，其"验证"已隐含在 **118 个 PASSED** 单元中。

## C 线 · 不覆盖(需真实驱动/多卡/专用硬件)
GE 图引擎、算子库 aolapi、HCCL、HIXL、ATB、SiP、DVPP —— 逐库的「为何不覆盖 + 将来覆盖条件」见 [`notcovered.md`](notcovered.md)。
