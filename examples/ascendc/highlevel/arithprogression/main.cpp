/*
 * Host: single-core launch of the ArithProgression (Arange) kernel.
 * firstValue=0, diffValue=1 -> dst[i] = i.
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

    float *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&zH, byteSize));

    uint8_t *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&zD, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, byteSize, zD, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int32_t i = 0; i < N; i++) {
        float ev = (float)i;
        if (fabsf(zH[i] - ev) > 1e-3f) {
            if (errors < 5) printf("[CHECK] z[%d]=%f (expect %f)\n", i, zH[i], ev);
            errors++;
        }
    }
    printf("z[0]=%f z[1]=%f z[63]=%f (expect 0,1,63) errors=%d\n", zH[0], zH[1], zH[63], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown.
    if (errors == 0) printf("ARITHPROGRESSION SIMULATION PASSED\n");
    else             printf("ARITHPROGRESSION SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
