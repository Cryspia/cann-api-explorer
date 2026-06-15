/*
 * Host: single-core launch of LayerNormGradBeta. [BS=4, H=8].
 * resForGamma=3.0, dy=2.0 (all constant). Reduce over BS:
 *   pd_gamma[h] = sum_bs(dy*resForGamma) = 4*(2*3) = 24.0
 *   pd_beta[h]  = sum_bs(dy)             = 4*2     = 8.0
 * Verified on host.
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
    const int32_t BS = 4, H = 8, TOTAL = BS * H;
    const size_t tBytes = (size_t)TOTAL * sizeof(float);
    const size_t hBytes = (size_t)H * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *rgHost = nullptr, *dyHost = nullptr, *pgHost = nullptr, *pbHost = nullptr;
    uint8_t *rgDev = nullptr, *dyDev = nullptr, *pgDev = nullptr, *pbDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&rgHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&dyHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&pgHost, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&pbHost, hBytes));
    CHECK_ACL(aclrtMalloc((void **)&rgDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&dyDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&pgDev, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&pbDev, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < TOTAL; i++) { rgHost[i] = 3.0f; dyHost[i] = 2.0f; }
    CHECK_ACL(aclrtMemcpy(rgDev, tBytes, rgHost, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(dyDev, tBytes, dyHost, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, rgDev, dyDev, pgDev, pbDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(pgHost, hBytes, pgDev, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(pbHost, hBytes, pbDev, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expectGamma = 24.0f, expectBeta = 8.0f;
    int errors = 0;
    for (int i = 0; i < H; i++) {
        if (fabsf(pgHost[i] - expectGamma) > 5e-3f) {
            if (errors < 5) printf("[CHECK] pdGamma idx %d = %f (expect %f)\n", i, pgHost[i], expectGamma);
            errors++;
        }
        if (fabsf(pbHost[i] - expectBeta) > 5e-3f) {
            if (errors < 5) printf("[CHECK] pdBeta idx %d = %f (expect %f)\n", i, pbHost[i], expectBeta);
            errors++;
        }
    }
    printf("pdGamma[0]=%f (expect %f) pdBeta[0]=%f (expect %f) H=%d errors=%d\n",
           pgHost[0], expectGamma, pbHost[0], expectBeta, H, errors);

    if (errors == 0) printf("LAYERNORMGRADBETA SIMULATION PASSED\n");
    else             printf("LAYERNORMGRADBETA SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(rgDev));
    CHECK_ACL(aclrtFree(dyDev));
    CHECK_ACL(aclrtFree(pgDev));
    CHECK_ACL(aclrtFree(pbDev));
    CHECK_ACL(aclrtFreeHost(rgHost));
    CHECK_ACL(aclrtFreeHost(dyHost));
    CHECK_ACL(aclrtFreeHost(pgHost));
    CHECK_ACL(aclrtFreeHost(pbHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
