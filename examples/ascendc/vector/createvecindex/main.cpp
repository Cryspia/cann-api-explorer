/* Host: CreateVecIndex, firstValue=0, N=64, expect dst[i]=i. */
#include <cstdio>
#include <cstdint>
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
    const int32_t N = 64;
    const size_t bytes = (size_t)N * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    uint8_t *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) if (zH[i] != (float)i) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %d)\n", i, zH[i], i); errors++; }
    printf("z[0..3]=[%g,%g,%g,%g] errors=%d\n", zH[0], zH[1], zH[2], zH[3], errors);

    CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());
    if (errors == 0) { printf("CREATEVECINDEX SIMULATION PASSED\n"); return 0; }
    printf("CREATEVECINDEX SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
