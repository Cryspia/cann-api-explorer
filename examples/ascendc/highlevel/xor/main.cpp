/* Host: BitwiseXor, src0=6, src1=3 (int32) -> expect dst = 6 ^ 3 = 5. Integer-exact verify. */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const size_t bytes = (size_t)N * sizeof(int32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *aH = nullptr, *bH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&aH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&bH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < N; i++) { aH[i] = 6; bH[i] = 3; }
    uint8_t *aD = nullptr, *bD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&aD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&bD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(aD, bytes, aH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(bD, bytes, bH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, aD, bD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const int32_t expect = 6 ^ 3;   // = 5
    int errors = 0;
    for (int i = 0; i < N; i++) { if (zH[i] != expect) { if (errors < 5) printf("[CHECK] z[%d]=%d (expect %d)\n", i, zH[i], expect); errors++; } }
    printf("z[0]=%d z[last]=%d (expect %d) errors=%d\n", zH[0], zH[N - 1], expect, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("XOR SIMULATION PASSED\n");
    else             printf("XOR SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(aD)); CHECK_ACL(aclrtFree(bD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(aH)); CHECK_ACL(aclrtFreeHost(bH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
