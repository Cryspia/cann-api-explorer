/*
 * Host: Conv3dTiling computes TConv3DApiTiling -> device launches the Conv3D kernel.
 * Cin=Cout=16, 1x1x1 kernel, Din=Hin=Win=1. fmap all 1.0 (half), weight all 0.5 -> output all 8.0.
 * half encodes hard-coded values via uint16: 1.0=0x3C00, 0.5=0x3800, 8.0=0x4800.
 */
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"
#include "tiling/platform/platform_ascendc.h"
#include "adv_api/conv/conv3d/conv3d_tiling.h"
#include "kernel_tiling/kernel_tiling.h"

#define CHECK_ACL(x)                                                                   \
    do {                                                                               \
        aclError __ret = (x);                                                          \
        if (__ret != ACL_SUCCESS) {                                                    \
            printf("[ERROR] %s:%d acl ret = %d\n", __FILE__, __LINE__, (int)__ret);    \
            return 1;                                                                  \
        }                                                                              \
    } while (0)

int32_t main()
{
    const int32_t Ci = 16, Co = 16, Kd = 1, Kh = 1, Kw = 1, Di = 1, Hi = 1, Wi = 1;
    const int32_t FMAP_ELEM = 16, WEIGHT_ELEM = 256, OUT_ELEM = 16;
    const uint16_t H_ONE = 0x3C00, H_HALF = 0x3800, H_EIGHT = 0x4800;

    // ---- host tiling ----
    auto *platform = platform_ascendc::PlatformAscendCManager::GetInstance();
    Conv3dTilingApi::Conv3dTiling tiling(*platform);
    tiling.SetInputType(ConvCommonApi::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, ConvCommonApi::ConvDtype::FLOAT16);
    tiling.SetWeightType(ConvCommonApi::TPosition::GM, ConvCommonApi::ConvFormat::FRACTAL_Z_3D, ConvCommonApi::ConvDtype::FLOAT16);
    tiling.SetOutputType(ConvCommonApi::TPosition::GM, ConvCommonApi::ConvFormat::NDC1HWC0, ConvCommonApi::ConvDtype::FLOAT16);
    tiling.SetOrgInputShape(Ci, Di, Hi, Wi);
    tiling.SetOrgWeightShape(Co, Kd, Kh, Kw);
    tiling.SetSingleWeightShape(Ci, Kd, Kh, Kw);
    tiling.SetSingleOutputShape(Co, 1 /*Do*/, 1 /*M=Ho*Wo*/);
    tiling.SetStride(1, 1, 1);
    tiling.SetDilation(1, 1, 1);
    tiling.SetPadding(0, 0, 0, 0, 0, 0);
    tiling.SetGroups(1);

    AscendC::tiling::TConv3DApiTiling t;
    int64_t ret = tiling.GetTiling(t);
    if (ret != 0) { printf("[ERROR] Conv3d GetTiling failed ret=%ld\n", (long)ret); return 1; }
    const size_t tBytes = sizeof(AscendC::tiling::TConv3DApiTiling);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    const size_t fBytes = (size_t)FMAP_ELEM * sizeof(uint16_t);
    const size_t wBytes = (size_t)WEIGHT_ELEM * sizeof(uint16_t);
    const size_t oBytes = (size_t)OUT_ELEM * sizeof(uint16_t);

    uint16_t *fH = nullptr, *wH = nullptr, *oH = nullptr; uint8_t *tH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&fH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&wH, wBytes));
    CHECK_ACL(aclrtMallocHost((void **)&oH, oBytes));
    CHECK_ACL(aclrtMallocHost((void **)&tH, tBytes));
    for (int i = 0; i < FMAP_ELEM; i++) fH[i] = H_ONE;
    for (int i = 0; i < WEIGHT_ELEM; i++) wH[i] = H_HALF;
    memcpy(tH, &t, tBytes);

    uint8_t *fD = nullptr, *wD = nullptr, *oD = nullptr, *tD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&fD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&wD, wBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&oD, oBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&tD, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(fD, fBytes, fH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(wD, wBytes, wH, wBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(tD, tBytes, tH, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, fD, wD, oD, tD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(oH, oBytes, oD, oBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < OUT_ELEM; i++) {
        if (oH[i] != H_EIGHT) {
            if (errors < 8) printf("[CHECK] out[%d]=0x%04X (expect 0x%04X=8.0)\n", i, oH[i], H_EIGHT);
            errors++;
        }
    }
    printf("out[0..2]=[0x%04X,0x%04X,0x%04X] expect=0x%04X errors=%d\n", oH[0], oH[1], oH[2], H_EIGHT, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("CONV3D SIMULATION PASSED\n");
    else             printf("CONV3D SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(fD)); CHECK_ACL(aclrtFree(wD)); CHECK_ACL(aclrtFree(oD)); CHECK_ACL(aclrtFree(tD));
    CHECK_ACL(aclrtFreeHost(fH)); CHECK_ACL(aclrtFreeHost(wH)); CHECK_ACL(aclrtFreeHost(oH)); CHECK_ACL(aclrtFreeHost(tH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
