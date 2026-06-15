/* Host: Quantize (PER_TENSOR). src=2.0, scale=2.0, offset=1.0
 * -> dst = round(2*2 + 1) = 5 (int8). */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

using ST = float;
using DT = int8_t;

int32_t main()
{
    const int32_t N = 256;
    const size_t sBytes = (size_t)N * sizeof(ST);
    const size_t dBytes = (size_t)N * sizeof(DT);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    ST *xH = nullptr; DT *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dBytes));
    for (int i = 0; i < N; i++) { xH[i] = (ST)2.0; zH[i] = (DT)0; }

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, sBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, sBytes, xH, sBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, dBytes, zD, dBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const DT expect = (DT)5;
    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (zH[i] != expect) { if (errors < 5) printf("[CHECK] z[%d]=%d (expect %d)\n", i, (int)zH[i], (int)expect); errors++; }
    }
    printf("z[0]=%d z[last]=%d (expect 5) errors=%d\n", (int)zH[0], (int)zH[N - 1], errors);

    if (errors == 0) printf("QUANTIZE SIMULATION PASSED\n");
    else             printf("QUANTIZE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
