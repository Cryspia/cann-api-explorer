/* Host: WelfordUpdate one step. inMean=1, inVar=0, x=3, nRec=0.5(n=2)
 * -> outMean=2, outVar=2 for every element. */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 8;
    const size_t bytes = (size_t)N * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *mH = nullptr, *vH = nullptr, *omH = nullptr, *ovH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&mH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&vH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&omH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&ovH, bytes));
    for (int i = 0; i < N; i++) { xH[i] = 3.0f; mH[i] = 1.0f; vH[i] = 0.0f; omH[i] = -1.0f; ovH[i] = -1.0f; }

    uint8_t *xD = nullptr, *mD = nullptr, *vD = nullptr, *omD = nullptr, *ovD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&mD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&vD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&omD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&ovD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(mD, bytes, mH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(vD, bytes, vH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, mD, vD, omD, ovD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(omH, bytes, omD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(ovH, bytes, ovD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (fabsf(omH[i] - 2.0f) > 1e-4f) { if (errors < 5) printf("[CHECK] outMean[%d]=%g (expect 2)\n", i, omH[i]); errors++; }
        if (fabsf(ovH[i] - 2.0f) > 1e-4f) { if (errors < 5) printf("[CHECK] outVar[%d]=%g (expect 2)\n", i, ovH[i]); errors++; }
    }
    printf("outMean[0]=%g outVar[0]=%g (expect 2, 2) errors=%d\n", omH[0], ovH[0], errors);

    if (errors == 0) printf("WELFORDUPDATE SIMULATION PASSED\n");
    else             printf("WELFORDUPDATE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(mD)); CHECK_ACL(aclrtFree(vD));
    CHECK_ACL(aclrtFree(omD)); CHECK_ACL(aclrtFree(ovD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(mH)); CHECK_ACL(aclrtFreeHost(vH));
    CHECK_ACL(aclrtFreeHost(omH)); CHECK_ACL(aclrtFreeHost(ovH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
