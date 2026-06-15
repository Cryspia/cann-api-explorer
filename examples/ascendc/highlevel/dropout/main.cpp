/*
 * Host: single-core launch of DropOut. src[64]=3.0, mask[64]=1 (keep all), keepProb=0.5.
 * Byte mode -> dst = (1/0.5)*1*3.0 = 6.0 everywhere. Deterministic (caller-supplied mask). Verified on host.
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
    const uint32_t blockDim = 1;
    const int32_t TOTAL = 64;
    const size_t fBytes = (size_t)TOTAL * sizeof(float);
    const size_t mBytes = (size_t)TOTAL * sizeof(uint8_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *srcHost = nullptr, *dstHost = nullptr;
    uint8_t *maskHost = nullptr;
    uint8_t *srcDev = nullptr, *maskDev = nullptr, *dstDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&srcHost, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&maskHost, mBytes));
    CHECK_ACL(aclrtMallocHost((void **)&dstHost, fBytes));
    CHECK_ACL(aclrtMalloc((void **)&srcDev, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&maskDev, mBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&dstDev, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < TOTAL; i++) { srcHost[i] = 3.0f; maskHost[i] = 1; }
    CHECK_ACL(aclrtMemcpy(srcDev, fBytes, srcHost, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(maskDev, mBytes, maskHost, mBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, srcDev, maskDev, dstDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(dstHost, fBytes, dstDev, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 6.0f;  // (1/keepProb)*mask*src = 2*1*3
    int errors = 0;
    for (int i = 0; i < TOTAL; i++) {
        if (fabsf(dstHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, dstHost[i], expect);
            errors++;
        }
    }
    printf("dst[0]=%f dst[last]=%f expect=%f total=%d errors=%d\n",
           dstHost[0], dstHost[TOTAL - 1], expect, TOTAL, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown so the simulator's stdout capture records it.
    if (errors == 0) printf("DROPOUT SIMULATION PASSED\n");
    else             printf("DROPOUT SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(srcDev));
    CHECK_ACL(aclrtFree(maskDev));
    CHECK_ACL(aclrtFree(dstDev));
    CHECK_ACL(aclrtFreeHost(srcHost));
    CHECK_ACL(aclrtFreeHost(maskHost));
    CHECK_ACL(aclrtFreeHost(dstHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
