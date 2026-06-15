/*
 * Host: single-core launch of LayerNormGrad. [B=1,S=1,H=8].
 * dy=0, x=mean=5.0, var=0, gamma=1.0, eps=1e-5.
 * dy=0 -> outputPdX=0 everywhere; x=mean -> resForGamma=(x-mean)*rstd=0 everywhere. Verified on host.
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
    const int32_t H = 8, BS_PAD = 8, TOTAL = 1 * 1 * 8;
    const size_t tBytes = (size_t)TOTAL * sizeof(float);
    const size_t hBytes = (size_t)H * sizeof(float);
    const size_t bsBytes = (size_t)BS_PAD * sizeof(float);  // mean/var padded to 32B for DataCopy

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *dyHost = nullptr, *xHost = nullptr, *varHost = nullptr, *meanHost = nullptr, *gammaHost = nullptr;
    float *pdxHost = nullptr, *rgHost = nullptr;
    uint8_t *dyDev = nullptr, *xDev = nullptr, *varDev = nullptr, *meanDev = nullptr, *gammaDev = nullptr;
    uint8_t *pdxDev = nullptr, *rgDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&dyHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&xHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&varHost, bsBytes));
    CHECK_ACL(aclrtMallocHost((void **)&meanHost, bsBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gammaHost, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&pdxHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&rgHost, tBytes));
    CHECK_ACL(aclrtMalloc((void **)&dyDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&xDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&varDev, bsBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&meanDev, bsBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gammaDev, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&pdxDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&rgDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < TOTAL; i++) { dyHost[i] = 0.0f; xHost[i] = 5.0f; }
    for (int i = 0; i < BS_PAD; i++) { varHost[i] = 0.0f; meanHost[i] = 5.0f; }
    for (int i = 0; i < H; i++) gammaHost[i] = 1.0f;
    CHECK_ACL(aclrtMemcpy(dyDev, tBytes, dyHost, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(xDev, tBytes, xHost, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(varDev, bsBytes, varHost, bsBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(meanDev, bsBytes, meanHost, bsBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gammaDev, hBytes, gammaHost, hBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, dyDev, xDev, varDev, meanDev, gammaDev, pdxDev, rgDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(pdxHost, tBytes, pdxDev, tBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(rgHost, tBytes, rgDev, tBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 0.0f;
    int errors = 0;
    for (int i = 0; i < TOTAL; i++) {
        if (fabsf(pdxHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] pdX idx %d = %f (expect %f)\n", i, pdxHost[i], expect);
            errors++;
        }
        if (fabsf(rgHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] resForGamma idx %d = %f (expect %f)\n", i, rgHost[i], expect);
            errors++;
        }
    }
    printf("pdX[0]=%f resForGamma[0]=%f expect=%f total=%d errors=%d\n",
           pdxHost[0], rgHost[0], expect, TOTAL, errors);

    if (errors == 0) printf("LAYERNORMGRAD SIMULATION PASSED\n");
    else             printf("LAYERNORMGRAD SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(dyDev));
    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(varDev));
    CHECK_ACL(aclrtFree(meanDev));
    CHECK_ACL(aclrtFree(gammaDev));
    CHECK_ACL(aclrtFree(pdxDev));
    CHECK_ACL(aclrtFree(rgDev));
    CHECK_ACL(aclrtFreeHost(dyHost));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(varHost));
    CHECK_ACL(aclrtFreeHost(meanHost));
    CHECK_ACL(aclrtFreeHost(gammaHost));
    CHECK_ACL(aclrtFreeHost(pdxHost));
    CHECK_ACL(aclrtFreeHost(rgHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
