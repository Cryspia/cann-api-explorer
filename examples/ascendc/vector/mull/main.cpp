/* Host: Mull, src0=2, src1=3 (int32) -> expect low=6, high=0. Only low half is verified. */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 256;
    const size_t bytes = (size_t)N * sizeof(int32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *x0H = nullptr, *x1H = nullptr, *loH = nullptr, *hiH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&loH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&hiH, bytes));
    for (int i = 0; i < N; i++) { x0H[i] = 2; x1H[i] = 3; }

    uint8_t *x0D = nullptr, *x1D = nullptr, *loD = nullptr, *hiD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&x0D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&x1D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&loD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&hiD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(x0D, bytes, x0H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(x1D, bytes, x1H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, x0D, x1D, loD, hiD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(loH, bytes, loD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(hiH, bytes, hiD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    const int32_t evLow = 6;
    for (int i = 0; i < N; i++) { if (loH[i] != evLow) { if (errors < 5) printf("[CHECK] low[%d]=%d (expect %d)\n", i, loH[i], evLow); errors++; } }
    printf("low[0..3]=[%d,%d,%d,%d] high[0]=%d (expect low=6) errors=%d\n", loH[0], loH[1], loH[2], loH[3], hiH[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("MULL SIMULATION PASSED\n");
    else             printf("MULL SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(loD)); CHECK_ACL(aclrtFree(hiD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(loH)); CHECK_ACL(aclrtFreeHost(hiH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
