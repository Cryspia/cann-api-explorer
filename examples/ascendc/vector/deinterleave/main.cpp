/*
 * Host: DeInterleave, src[i]=i (interleaved stream [0,1,2,3,...]).
 * Split into dst0 (even positions) = [0,2,4,...] and dst1 (odd positions) = [1,3,5,...].
 * Verify dst0[i]==2*i and dst1[i]==2*i+1 for i in [0, N/2).
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
    const int32_t H = N / 2;
    const size_t bytes = (size_t)N * sizeof(float);
    const size_t hBytes = (size_t)H * sizeof(float);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *z0H = nullptr, *z1H = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&z0H, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&z1H, hBytes));
    for (int i = 0; i < N; i++) xH[i] = (float)i;
    uint8_t *xD = nullptr, *z0D = nullptr, *z1D = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&z0D, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&z1D, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, z0D, z1D);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(z0H, hBytes, z0D, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(z1H, hBytes, z1D, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < H; i++) {
        float e0 = (float)(2 * i);
        float e1 = (float)(2 * i + 1);
        if (z0H[i] != e0) { if (errors < 5) printf("[CHECK] dst0[%d]=%g (expect %g)\n", i, z0H[i], e0); errors++; }
        if (z1H[i] != e1) { if (errors < 5) printf("[CHECK] dst1[%d]=%g (expect %g)\n", i, z1H[i], e1); errors++; }
    }
    printf("dst0[0..2]=[%g,%g,%g] dst1[0..2]=[%g,%g,%g] errors=%d\n",
           z0H[0], z0H[1], z0H[2], z1H[0], z1H[1], z1H[2], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("DEINTERLEAVE SIMULATION PASSED\n");
    else             printf("DEINTERLEAVE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(z0D)); CHECK_ACL(aclrtFree(z1D));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(z0H)); CHECK_ACL(aclrtFreeHost(z1H));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
