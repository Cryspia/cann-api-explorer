/*
 * Host: launches the BatchNorm kernel (BatchNormTiling has 3 fields hand-filled inside the kernel, no host tiling).
 * [B=4,F=8], x all 5, gamma=1, beta=2, eps=1e-5.
 * Normalized along the B dim: the 4 batch values per feature are equal -> var=0 -> output=beta=2, mean=5.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

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
    const int32_t B = 4, F = 8, BF = B * F;
    const size_t xBytes = (size_t)BF * sizeof(float);
    const size_t fBytes = (size_t)F * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *gammaH = nullptr, *betaH = nullptr, *zH = nullptr, *meanH = nullptr, *varH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, xBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gammaH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&betaH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, xBytes));
    CHECK_ACL(aclrtMallocHost((void **)&meanH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&varH, fBytes));
    for (int i = 0; i < BF; i++) xH[i] = 5.0f;
    for (int f = 0; f < F; f++) { gammaH[f] = 1.0f; betaH[f] = 2.0f; }

    uint8_t *xD = nullptr, *gammaD = nullptr, *betaD = nullptr, *zD = nullptr, *meanD = nullptr, *varD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, xBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gammaD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&betaD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, xBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&meanD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&varD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, xBytes, xH, xBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gammaD, fBytes, gammaH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(betaD, fBytes, betaH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, gammaD, betaD, zD, meanD, varD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, xBytes, zD, xBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(meanH, fBytes, meanD, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < BF; i++) {
        if (fabsf(zH[i] - 2.0f) > 1e-3f) {
            if (errors < 8) printf("[CHECK] out[%d]=%g (expect 2)\n", i, zH[i]);
            errors++;
        }
    }
    if (fabsf(meanH[0] - 5.0f) > 1e-3f) { printf("[CHECK] mean[0]=%g (expect 5)\n", meanH[0]); errors++; }
    printf("out[0..2]=[%g,%g,%g] mean[0]=%g errors=%d\n", zH[0], zH[1], zH[2], meanH[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("BATCHNORM SIMULATION PASSED\n");
    else             printf("BATCHNORM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(gammaD)); CHECK_ACL(aclrtFree(betaD));
    CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFree(meanD)); CHECK_ACL(aclrtFree(varD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(gammaH)); CHECK_ACL(aclrtFreeHost(betaH));
    CHECK_ACL(aclrtFreeHost(zH)); CHECK_ACL(aclrtFreeHost(meanH)); CHECK_ACL(aclrtFreeHost(varH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
