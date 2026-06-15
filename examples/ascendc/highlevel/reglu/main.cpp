/*
 * Host: single-core launch of the ReGlu kernel.
 * src0 = 2.0, src1 = 3.0 -> dst = src0 * max(0, src1) = 2 * 3 = 6.0.
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
    const int32_t N = 64;
    const size_t byteSize = (size_t)N * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *x0H = nullptr, *x1H = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zH, byteSize));
    for (int32_t i = 0; i < N; i++) { x0H[i] = 2.0f; x1H[i] = 3.0f; }

    uint8_t *x0D = nullptr, *x1D = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&x0D, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&x1D, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(x0D, byteSize, x0H, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(x1D, byteSize, x1H, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, x0D, x1D, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, byteSize, zD, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 6.0f;       // 2 * max(0, 3)
    int errors = 0;
    for (int32_t i = 0; i < N; i++) {
        if (fabsf(zH[i] - expect) > 1e-3f) {
            if (errors < 5) printf("[CHECK] z[%d]=%f (expect %f)\n", i, zH[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[63]=%f expect=%f errors=%d\n", zH[0], zH[63], expect, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown.
    if (errors == 0) printf("REGLU SIMULATION PASSED\n");
    else             printf("REGLU SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
