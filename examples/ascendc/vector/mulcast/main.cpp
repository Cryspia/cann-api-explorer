/* Host: MulCast, src0[i]=i, src1[i]=2 (int64) -> expect dst[i]=2i (int32). */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const size_t sBytes = (size_t)N * sizeof(int64_t);
    const size_t dBytes = (size_t)N * sizeof(int32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int64_t *x0H = nullptr, *x1H = nullptr; int32_t *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dBytes));
    for (int i = 0; i < N; i++) { x0H[i] = (int64_t)i; x1H[i] = 2; }
    uint8_t *x0D = nullptr, *x1D = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&x0D, sBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&x1D, sBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(x0D, sBytes, x0H, sBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(x1D, sBytes, x1H, sBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, x0D, x1D, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, dBytes, zD, dBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) { int32_t ev = 2 * i; if (zH[i] != ev) { if (errors < 5) printf("[CHECK] z[%d]=%d (expect %d)\n", i, zH[i], ev); errors++; } }
    printf("z[0..3]=[%d,%d,%d,%d] errors=%d\n", zH[0], zH[1], zH[2], zH[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("MULCAST SIMULATION PASSED\n");
    else             printf("MULCAST SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
