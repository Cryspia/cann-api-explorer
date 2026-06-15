/*
 * Host: single-core launch of Normalize.
 * A=2 rows, R=8 cols. mean=0, variance=1, eps=0 -> rstd = rsqrt(1) = 1.
 * x=1 everywhere, gamma=1, beta=0 -> output = (1-0)*1*1+0 = 1; outputRstd = 1. Verified on host.
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
    const int32_t A = 2, APAD = 8, R = 8, RPAD = 8, TOTAL = A * RPAD;
    const size_t tBytes = (size_t)TOTAL * sizeof(float);
    const size_t aBytes = (size_t)APAD * sizeof(float);   // [A]-shaped buffers padded to 32B for DataCopy
    const size_t rBytes = (size_t)R * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *mH = nullptr, *vH = nullptr, *gH = nullptr, *bH = nullptr, *yH = nullptr, *rH = nullptr;
    uint8_t *xD = nullptr, *mD = nullptr, *vD = nullptr, *gD = nullptr, *bD = nullptr, *yD = nullptr, *rD = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&mH, aBytes));
    CHECK_ACL(aclrtMallocHost((void **)&vH, aBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gH, rBytes));
    CHECK_ACL(aclrtMallocHost((void **)&bH, rBytes));
    CHECK_ACL(aclrtMallocHost((void **)&yH, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&rH, aBytes));
    CHECK_ACL(aclrtMalloc((void **)&xD, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&mD, aBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&vD, aBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gD, rBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&bD, rBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&yD, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&rD, aBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < TOTAL; i++) xH[i] = 1.0f;
    for (int i = 0; i < APAD; i++) { mH[i] = 0.0f; vH[i] = 1.0f; }  // pad entries kept valid (var=1) too
    for (int i = 0; i < R; i++) { gH[i] = 1.0f; bH[i] = 0.0f; }
    CHECK_ACL(aclrtMemcpy(xD, tBytes, xH, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(mD, aBytes, mH, aBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(vD, aBytes, vH, aBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gD, rBytes, gH, rBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(bD, rBytes, bH, rBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xD, mD, vD, gD, bD, yD, rD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(yH, tBytes, yD, tBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(rH, aBytes, rD, aBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expectY = 1.0f, expectR = 1.0f;
    int errors = 0;
    for (int a = 0; a < A; a++) {
        for (int r = 0; r < R; r++) {
            float val = yH[a * RPAD + r];
            if (fabsf(val - expectY) > 5e-3f) {
                if (errors < 5) printf("[CHECK] y[%d,%d]=%f (expect %f)\n", a, r, val, expectY);
                errors++;
            }
        }
        if (fabsf(rH[a] - expectR) > 5e-3f) {
            if (errors < 5) printf("[CHECK] rstd[%d]=%f (expect %f)\n", a, rH[a], expectR);
            errors++;
        }
    }
    printf("y[0,0]=%f y[1,7]=%f rstd[0]=%f rstd[1]=%f expectY=%f expectR=%f errors=%d\n",
           yH[0], yH[RPAD + 7], rH[0], rH[1], expectY, expectR, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("NORMALIZE SIMULATION PASSED\n");
    else             printf("NORMALIZE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(mD)); CHECK_ACL(aclrtFree(vD));
    CHECK_ACL(aclrtFree(gD)); CHECK_ACL(aclrtFree(bD)); CHECK_ACL(aclrtFree(yD)); CHECK_ACL(aclrtFree(rD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(mH)); CHECK_ACL(aclrtFreeHost(vH));
    CHECK_ACL(aclrtFreeHost(gH)); CHECK_ACL(aclrtFreeHost(bH)); CHECK_ACL(aclrtFreeHost(yH)); CHECK_ACL(aclrtFreeHost(rH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
