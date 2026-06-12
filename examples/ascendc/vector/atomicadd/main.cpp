/* Host: SetAtomicAdd, 8 cores each add all 1.0 into the same GM -> expect z[i]=8.0 (GM must be zeroed). */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const uint32_t blockDim = 8;
    const size_t bytes = (size_t)N * sizeof(float);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < N; i++) zH[i] = 0.0f;   // zero out
    uint8_t *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(zD, bytes, zH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) if (fabsf(zH[i] - 8.0f) > 1e-3f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect 8)\n", i, zH[i]); errors++; }
    printf("z[0..2]=[%g,%g,%g] errors=%d\n", zH[0], zH[1], zH[2], errors);

    CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());
    if (errors == 0) { printf("ATOMICADD SIMULATION PASSED\n"); return 0; }
    printf("ATOMICADD SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
