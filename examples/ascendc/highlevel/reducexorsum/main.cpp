/*
 * Host: single-core launch of the ReduceXorSum kernel.
 * Inputs are two int16 vectors of length 64:
 *   src0[i] = i  (0..63),  src1[i] = 5  (constant).
 * Expected result = sum_i(src0[i] ^ src1[i]), computed identically on the host.
 * The scalar result lands in z[0].
 */
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
    const size_t bytes = (size_t)N * sizeof(int16_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    int16_t *x0H = nullptr, *x1H = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));

    int32_t expect = 0;
    for (int32_t i = 0; i < N; i++) {
        x0H[i] = (int16_t)i;
        x1H[i] = (int16_t)5;
        expect += (int32_t)((int16_t)(x0H[i] ^ x1H[i]));
    }

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
    if ((int32_t)zH[0] != expect) { printf("[CHECK] z[0]=%d (expect %d)\n", (int)zH[0], expect); errors++; }
    printf("z[0]=%d expect=%d errors=%d\n", (int)zH[0], expect, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown.
    if (errors == 0) printf("REDUCEXORSUM SIMULATION PASSED\n");
    else             printf("REDUCEXORSUM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
