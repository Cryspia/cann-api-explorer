/*
 * Host: TopKTilingFunc computes TopkTiling -> copy to device -> launch the TopK kernel.
 * src=[0..31] float, indices=[0..31], take the largest 4 -> values=[31,30,29,28], indices=[31,30,29,28].
 */
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"
#include "tiling/platform/platform_ascendc.h"
#include "tiling/topk/topk_tiling.h"
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
    const int32_t N = 32, INNER = 32, OUTTER = 1, K = 4;
    const size_t xBytes = (size_t)N * sizeof(float);
    const size_t iBytes = (size_t)N * sizeof(int32_t);
    const size_t zBytes = (size_t)INNER * sizeof(float);
    const size_t ziBytes = (size_t)INNER * sizeof(int32_t);

    // ---- TopkTiling ----
    auto *platform = platform_ascendc::PlatformAscendCManager::GetInstance();
    AscendC::tiling::TopkTiling tiling;
    bool ok = AscendC::TopKTilingFunc(*platform, INNER, OUTTER, K, sizeof(float),
                                      /*isInitIndex*/ true, AscendC::TopKMode::TOPK_NORMAL,
                                      /*isLargest*/ true, tiling);
    if (!ok) { printf("[ERROR] TopKTilingFunc failed\n"); return 1; }
    const size_t tBytes = sizeof(AscendC::tiling::TopkTiling);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr; int32_t *iH = nullptr, *ziH = nullptr; uint8_t *tH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, xBytes));
    CHECK_ACL(aclrtMallocHost((void **)&iH, iBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, zBytes));
    CHECK_ACL(aclrtMallocHost((void **)&ziH, ziBytes));
    CHECK_ACL(aclrtMallocHost((void **)&tH, tBytes));
    for (int i = 0; i < N; i++) { xH[i] = (float)i; iH[i] = i; }
    memcpy(tH, &tiling, tBytes);

    uint8_t *xD = nullptr, *iD = nullptr, *zD = nullptr, *ziD = nullptr, *tD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, xBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&iD, iBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, zBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&ziD, ziBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&tD, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, xBytes, xH, xBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(iD, iBytes, iH, iBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(tD, tBytes, tH, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, iD, zD, ziD, tD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, zBytes, zD, zBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(ziH, ziBytes, ziD, ziBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    float expectV[K] = {31, 30, 29, 28};
    int32_t expectI[K] = {31, 30, 29, 28};
    for (int i = 0; i < K; i++) {
        if (zH[i] != expectV[i] || ziH[i] != expectI[i]) {
            printf("[CHECK] top%d val=%f idx=%d (expect %f,%d)\n", i, zH[i], ziH[i], expectV[i], expectI[i]);
            errors++;
        }
    }
    printf("top4 val=[%g,%g,%g,%g] idx=[%d,%d,%d,%d] errors=%d\n",
           zH[0], zH[1], zH[2], zH[3], ziH[0], ziH[1], ziH[2], ziH[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("TOPK SIMULATION PASSED\n");
    else             printf("TOPK SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(iD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFree(ziD)); CHECK_ACL(aclrtFree(tD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(iH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtFreeHost(ziH)); CHECK_ACL(aclrtFreeHost(tH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
