/*
 * Hand-written advanced sample: Conv3D (Cube + host-side tiling framework, the most complex).
 * Host Conv3dTiling computes TConv3DApiTiling -> passed in via GM -> kernel copies it back -> Conv3D object Init/SetInput/SetWeight/IterateAll.
 * Simplest case: N=1, Cin=Cout=16 (C1=1, C0=16), 1x1x1 kernel, Din=Hin=Win=1, stride1/pad0/dilation1/groups1.
 * fmap NDC1HWC0 all 1.0, weight FRACTAL_Z_3D all 0.5 (all equal -> output is independent of the fractal layout).
 * output[co] = sum_{ci=0..15} fmap[ci]*weight[co][ci] = 16 * 1.0 * 0.5 = 8.0 (NDC1HWC0, 16 half values).
 */
#include "kernel_operator.h"
#include "adv_api/conv/conv3d/conv3d_api.h"

using AT = ConvApi::ConvType<AscendC::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, half>;
using BT = ConvApi::ConvType<AscendC::TPosition::GM, ConvCommonApi::ConvFormat::FRACTAL_Z_3D, half>;
using CT = ConvApi::ConvType<AscendC::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, half>;

constexpr uint32_t FMAP_ELEM = 16;     // NDC1HWC0 [1,1,1,1,1,16]
constexpr uint32_t WEIGHT_ELEM = 256;  // FRACTAL_Z_3D 1x1x1 16x16
constexpr uint32_t OUT_ELEM = 16;      // NDC1HWC0 [1,1,1,1,1,16]

extern "C" __global__ __aicore__ void k_custom(GM_ADDR fmap, GM_ADDR weight, GM_ADDR out, GM_ADDR tilingGm)
{
    // Copy back the TConv3DApiTiling computed on the host from GM
    AscendC::tiling::TConv3DApiTiling t;
    auto src = reinterpret_cast<__gm__ uint32_t *>(tilingGm);
    auto dst = reinterpret_cast<uint32_t *>(&t);
    for (uint32_t i = 0; i < sizeof(AscendC::tiling::TConv3DApiTiling) / sizeof(uint32_t); i++) dst[i] = src[i];

    AscendC::GlobalTensor<half> fmapGm, weightGm, outGm;
    fmapGm.SetGlobalBuffer((__gm__ half *)fmap, FMAP_ELEM);
    weightGm.SetGlobalBuffer((__gm__ half *)weight, WEIGHT_ELEM);
    outGm.SetGlobalBuffer((__gm__ half *)out, OUT_ELEM);

    Conv3dApi::Conv3D<AT, BT, CT> conv;
    conv.Init(&t);
    conv.SetInput(fmapGm);
    conv.SetWeight(weightGm);
    conv.IterateAll(outGm);
    conv.End();
}
