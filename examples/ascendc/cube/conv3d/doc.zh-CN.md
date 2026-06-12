# Ascend C · Conv3D（Cube，**本架构 3510 不支持** ⚠️）

- 分类：Cube / 卷积
- 覆盖 API：`Conv3dApi::Conv3D` + host `Conv3dTilingApi::Conv3dTiling`
- include：`adv_api/conv/conv3d/conv3d_api.h`（kernel）、`adv_api/conv/conv3d/conv3d_tiling.h`（host）
- 状态：**已完整实现接口代码，但本机仿真架构（3510 / Ascend950PR）无 Conv3D kernel 实现，无法编译** → 记为 future work。

## 结论：CANN 9.1.0-beta.1 的 Conv3D 仅有 m220（V220/910 系）实现
编译报错根因（非样例代码问题）：
```
impl/adv_api/detail/conv/conv3d/conv3d_api_impl.h:70: error:
  no member named 'LoadAL1WithPointWiseTools' in namespace 'Conv3dApiFunc'
```
排查结论：
- `impl/adv_api/detail/conv/conv3d/` 下**只有 `dav_m220/` 一个架构目录，零 3510 引用**。
- `dav_m220/conv3d_sub_api.h` 全程基于 `AscendC::FixpipeParamsV220`（V220/910 系硬件专用 fixpipe 原语）。
- 本机 `__NPU_ARCH__==3510`（Ascend950PR）用不同的 fixpipe，工具函数 `Conv3dApiFunc::LoadAL1Tools/MMadTools/...` 在 3510 下不展开 → 全部 "no member" 报错。
- 对比：Matmul（Cube）在 3510 能跑通（其 impl 非 dav_m220 独占组织）；Conv3D 则是 m220 独占。

## 已完成的接口实现（代码留存，待 m220 环境验证）
样例代码（`kernel.cpp` / `main.cpp` / `CMakeLists.txt`）已按 Matmul 同款 host-tiling 框架完整写出，结构正确，可作接口参考：
- host：`Conv3dTilingApi::Conv3dTiling(*platform)` → `SetInputType/SetWeightType/SetOutputType`(NDC1HWC0 / FRACTAL_Z_3D / FLOAT16) → `SetOrgInputShape/SetOrgWeightShape/SetSingleWeightShape/SetSingleOutputShape/SetStride/SetDilation/SetPadding/SetGroups` → `GetTiling(AscendC::tiling::TConv3DApiTiling&)` → memcpy 到 GM。
- kernel：从 GM 拷回 tiling → `Conv3dApi::Conv3D<AT,BT,CT> conv; conv.Init(&t); conv.SetInput(fmapGm); conv.SetWeight(weightGm); conv.IterateAll(outGm); conv.End();`，类型 `ConvApi::ConvType<AscendC::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, half>` 等。
- 最简设计（规避分形布局校验）：N=1, Cin=Cout=16, 1×1×1 kernel, D=H=W=1；**fmap 全 1.0、weight 全 0.5 全等 → 输出 = 16×1×0.5 = 8.0，与 NDC1HWC0/FRACTAL_Z_3D 重排无关**。

## 如何将来跑通
需要 m220（如 Ascend910B）的仿真/真机环境（构建 SOC 切到 V220 系），或等待 toolkit 为 3510 补 Conv3D 实现。
