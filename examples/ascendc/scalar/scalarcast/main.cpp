/* Host: ScalarCast 3.7f -> int (CAST_RINT) -> 4, expect z all 4. */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 8;
    const size_t bytes = (size_t)N * sizeof(int32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    uint8_t *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) if (zH[i] != 4) { if (errors < 5) printf("[CHECK] z[%d]=%d (expect 4)\n", i, zH[i]); errors++; }
    printf("z[0..2]=[%d,%d,%d] (3.7 round → 4) errors=%d\n", zH[0], zH[1], zH[2], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SCALARCAST SIMULATION PASSED\n");
    else             printf("SCALARCAST SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
