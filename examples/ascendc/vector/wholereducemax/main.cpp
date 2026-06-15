/* Host launcher for WholeReduceMax: Max of all 64 floats in each repeat (value-only order). */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do {                                                                              \
        aclError __ret = (x);                                                          \
        if (__ret != ACL_SUCCESS) {                                                   \
            printf("[ERROR] %s:%d acl ret = %d\n", __FILE__, __LINE__, (int)__ret);    \
            return 1;                                                                  \
        }                                                                             \
    } while (0)

int32_t main()
{
    const int32_t SRC = 512;
    const int32_t DSTPAD = 8;   // padded (32B-aligned) output length copied from device
    const int32_t NOUT = 8;       // number of meaningful output elements to verify
    const size_t srcBytes = (size_t)SRC * sizeof(float);
    const size_t dstBytes = (size_t)DSTPAD * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, srcBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dstBytes));
    for (int i = 0; i < SRC; i++) xH[i] = (float)(i % 64);

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, srcBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dstBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, srcBytes, xH, srcBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, dstBytes, zD, dstBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < 8; i++) { float ev = (float)(63.0); if (fabsf(zH[i]-ev) > 1e-3f) { if (errors<5) printf("[CHECK] z[%d]=%g (expect %g)\n", i, zH[i], ev); errors++; } }

    printf("z[0..2]=[%g,%g,%g] errors=%d\n", zH[0], NOUT>1?zH[1]:0.0f, NOUT>2?zH[2]:0.0f, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown.
    if (errors == 0) printf("WHOLEREDUCEMAX SIMULATION PASSED\n");
    else             printf("WHOLEREDUCEMAX SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
