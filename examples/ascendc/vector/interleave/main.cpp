/*
 * Host: Interleave, src0=all 1.0, src1=all 2.0.
 * dst0 holds the interleaved first N/2 pairs, dst1 the interleaved second N/2 pairs.
 * Concatenating [dst0, dst1] yields the full interleaved stream [1,2,1,2,...] of length 2N.
 * Verify every even index == 1 and every odd index == 2.
 */
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

    float *z0H = nullptr, *z1H = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&z0H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&z1H, bytes));
    uint8_t *z0D = nullptr, *z1D = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&z0D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&z1D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, z0D, z1D);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(z0H, bytes, z0D, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(z1H, bytes, z1D, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    // Concatenate the two halves into the full interleaved stream of length 2N.
    int errors = 0;
    for (int i = 0; i < 2 * N; i++) {
        float v = (i < N) ? z0H[i] : z1H[i - N];
        float ev = (i % 2 == 0) ? 1.0f : 2.0f;
        if (v != ev) { if (errors < 5) printf("[CHECK] out[%d]=%g (expect %g)\n", i, v, ev); errors++; }
    }
    printf("out[0..3]=[%g,%g,%g,%g] (expect 1,2,1,2) errors=%d\n", z0H[0], z0H[1], z0H[2], z0H[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("INTERLEAVE SIMULATION PASSED\n");
    else             printf("INTERLEAVE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(z0D)); CHECK_ACL(aclrtFree(z1D));
    CHECK_ACL(aclrtFreeHost(z0H)); CHECK_ACL(aclrtFreeHost(z1H));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
