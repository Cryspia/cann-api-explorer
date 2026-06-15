/*
 * Host: single-core launch of the Sum kernel. Input [1,64] all 1.0,
 * last-axis sum over 64 elements -> dst[0] = 64.0.
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
    const int32_t IN_ELEM = 64;
    const int32_t OUT_ELEM = 64;  // aligned copy-out buffer; the sum lands in z[0]
    const size_t inBytes = (size_t)IN_ELEM * sizeof(float);
    const size_t outBytes = (size_t)OUT_ELEM * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, inBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, outBytes));
    for (int32_t i = 0; i < IN_ELEM; i++) xH[i] = 1.0f;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, inBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, outBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, inBytes, xH, inBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, outBytes, zD, outBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 64.0f;       // sum of 64 ones
    int errors = 0;
    if (fabsf(zH[0] - expect) > 1e-3f) { printf("[CHECK] z[0]=%f (expect %f)\n", zH[0], expect); errors++; }
    printf("z[0]=%f expect=%f errors=%d\n", zH[0], expect, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown.
    if (errors == 0) printf("SUM SIMULATION PASSED\n");
    else             printf("SUM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
