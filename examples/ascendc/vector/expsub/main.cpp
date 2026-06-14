/* Host: ExpSub, src0=1, src1=1 -> expect dst = exp(1-1) = exp(0) = 1. */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 256;
    const size_t bytes = (size_t)N * sizeof(float);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *x0H = nullptr, *x1H = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < N; i++) { x0H[i] = 1.0f; x1H[i] = 1.0f; }

    uint8_t *x0D = nullptr, *x1D = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&x0D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&x1D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(x0D, bytes, x0H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(x1D, bytes, x1H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, x0D, x1D, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    const float ev = 1.0f;
    for (int i = 0; i < N; i++) { if (fabsf(zH[i] - ev) > 1e-2f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %g)\n", i, zH[i], ev); errors++; } }
    printf("z[0..3]=[%g,%g,%g,%g] (expect 1) errors=%d\n", zH[0], zH[1], zH[2], zH[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("EXPSUB SIMULATION PASSED\n");
    else             printf("EXPSUB SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
