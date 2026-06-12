/* Host: Gather, src[i]=i, srcOffset[i]=(N-1-i)*4 bytes -> expect dst[i]=N-1-i (reverse order). */
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
    const int32_t N = 64;
    const size_t fBytes = (size_t)N * sizeof(float);
    const size_t oBytes = (size_t)N * sizeof(uint32_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr; uint32_t *oH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&oH, oBytes));
    for (int i = 0; i < N; i++) { xH[i] = (float)i; oH[i] = (uint32_t)((N - 1 - i) * sizeof(float)); }

    uint8_t *xD = nullptr, *oD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&oD, oBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, fBytes, xH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(oD, oBytes, oH, oBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, oD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, fBytes, zD, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) { float ev = (float)(N - 1 - i); if (zH[i] != ev) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %g)\n", i, zH[i], ev); errors++; } }
    printf("z[0..2]=[%g,%g,%g] (expect 63,62,61) errors=%d\n", zH[0], zH[1], zH[2], errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(oD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(oH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());
    if (errors == 0) { printf("GATHER SIMULATION PASSED\n"); return 0; }
    printf("GATHER SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
