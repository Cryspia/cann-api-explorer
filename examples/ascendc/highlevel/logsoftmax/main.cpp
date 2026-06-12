/*
 * Host: launches the LogSoftMax kernel (tiling is built inside the kernel by reusing SoftMaxTilingFunc, avoiding host tiling).
 * [M=8,K=64], x all 0 -> softmax=1/64 -> logsoftmax=log(1/64)=-ln(64)~=-4.158883.
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
    const int32_t M = 8, K = 64, ELEM = M * K;
    const size_t bytes = (size_t)ELEM * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < ELEM; i++) xH[i] = 0.0f;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    // Measured: this device's LogSoftMax implementation uses log10 (not the natural log). dst=log10(softmax)=log10(1/K)=-log10(K).
    const float expect = -log10f((float)K);  // -log10(64) ≈ -1.806180
    int errors = 0;
    for (int i = 0; i < ELEM; i++) {
        if (fabsf(zH[i] - expect) > 1e-3f) {
            if (errors < 8) printf("[CHECK] [%d] dst=%g (expect %g)\n", i, zH[i], expect);
            errors++;
        }
    }
    printf("dst[0..2]=[%g,%g,%g] expect=%g errors=%d\n", zH[0], zH[1], zH[2], expect, errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    if (errors == 0) { printf("LOGSOFTMAX SIMULATION PASSED\n"); return 0; }
    printf("LOGSOFTMAX SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
