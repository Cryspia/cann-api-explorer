# Ascend C · Conv3D (Cube, **not supported on this 3510 architecture** ⚠️)

- Category: Cube / convolution
- APIs covered: `Conv3dApi::Conv3D` + host `Conv3dTilingApi::Conv3dTiling`
- include: `adv_api/conv/conv3d/conv3d_api.h` (kernel), `adv_api/conv/conv3d/conv3d_tiling.h` (host)
- Status: **The interface code is fully implemented, but the local simulation architecture (3510 / Ascend950PR) has no Conv3D kernel implementation and cannot compile** -> recorded as future work.

## Conclusion: Conv3D in CANN 9.1.0-beta.1 only has the m220 (V220/910 series) implementation
Root cause of the compile error (not a problem with the sample code):
```
impl/adv_api/detail/conv/conv3d/conv3d_api_impl.h:70: error:
  no member named 'LoadAL1WithPointWiseTools' in namespace 'Conv3dApiFunc'
```
Investigation findings:
- Under `impl/adv_api/detail/conv/conv3d/` there is **only one architecture directory, `dav_m220/`, with zero references to 3510**.
- `dav_m220/conv3d_sub_api.h` is built entirely on `AscendC::FixpipeParamsV220` (a fixpipe primitive specific to V220/910 series hardware).
- The local `__NPU_ARCH__==3510` (Ascend950PR) uses a different fixpipe, so the helper functions `Conv3dApiFunc::LoadAL1Tools/MMadTools/...` do not expand under 3510 -> all give "no member" errors.
- For comparison: Matmul (Cube) runs fine on 3510 (its impl is not organized exclusively under dav_m220); Conv3D, however, is m220-exclusive.

## Completed interface implementation (code retained, pending validation on an m220 environment)
The sample code (`kernel.cpp` / `main.cpp` / `CMakeLists.txt`) is fully written following the same host-side tiling framework as Matmul; the structure is correct and can serve as an interface reference:
- host: `Conv3dTilingApi::Conv3dTiling(*platform)` -> `SetInputType/SetWeightType/SetOutputType` (NDC1HWC0 / FRACTAL_Z_3D / FLOAT16) -> `SetOrgInputShape/SetOrgWeightShape/SetSingleWeightShape/SetSingleOutputShape/SetStride/SetDilation/SetPadding/SetGroups` -> `GetTiling(AscendC::tiling::TConv3DApiTiling&)` -> memcpy to GM.
- kernel: copy tiling back from GM -> `Conv3dApi::Conv3D<AT,BT,CT> conv; conv.Init(&t); conv.SetInput(fmapGm); conv.SetWeight(weightGm); conv.IterateAll(outGm); conv.End();`, with types such as `ConvApi::ConvType<AscendC::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, half>`.
- Simplest design (avoiding fractal-layout verification): N=1, Cin=Cout=16, 1×1×1 kernel, D=H=W=1; **fmap all 1.0, weight all 0.5, all equal -> output = 16×1×0.5 = 8.0, independent of the NDC1HWC0/FRACTAL_Z_3D reordering**.

## How to run it in the future
This requires an m220 (e.g. Ascend910B) simulation/real-hardware environment (switch the build SOC to the V220 series), or wait for the toolkit to add a Conv3D implementation for 3510.
