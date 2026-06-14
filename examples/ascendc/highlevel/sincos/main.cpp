/* Host: SinCos, src all 0.0 -> expect dstSin ~ 0.0 and dstCos ~ 1.0. Both outputs verified. */
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
    const size_t bytes = (size_t)N * sizeof(float);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *sinH = nullptr, *cosH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&sinH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&cosH, bytes));
    for (int i = 0; i < N; i++) xH[i] = 0.0f;
    uint8_t *xD = nullptr, *sinD = nullptr, *cosD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&sinD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&cosD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, sinD, cosD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(sinH, bytes, sinD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(cosH, bytes, cosD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (fabsf(sinH[i] - 0.0f) > 5e-3f) { if (errors < 5) printf("[CHECK] sin[%d]=%g (expect 0)\n", i, sinH[i]); errors++; }
        if (fabsf(cosH[i] - 1.0f) > 5e-3f) { if (errors < 5) printf("[CHECK] cos[%d]=%g (expect 1)\n", i, cosH[i]); errors++; }
    }
    printf("sin[0]=%g cos[0]=%g errors=%d\n", sinH[0], cosH[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SINCOS SIMULATION PASSED\n");
    else             printf("SINCOS SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(sinD)); CHECK_ACL(aclrtFree(cosD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(sinH)); CHECK_ACL(aclrtFreeHost(cosH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
