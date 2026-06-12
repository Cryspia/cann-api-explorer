/* Host: GatherMask diagnosis, src[i]=i, pattern=1 -> dump dst + rsvdCnt. */
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const size_t bytes = (size_t)N * sizeof(float);
    const size_t rBytes = (size_t)8 * sizeof(uint32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr; uint32_t *rH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&rH, rBytes));
    for (int i = 0; i < N; i++) { xH[i] = (float)i; zH[i] = -1.0f; }
    uint8_t *xD = nullptr, *zD = nullptr, *rD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&rD, rBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(zD, bytes, zH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD, rD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(rH, rBytes, rD, rBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    printf("rsvdCnt=%u  dst[0..7]=[%g,%g,%g,%g,%g,%g,%g,%g]\n", rH[0], zH[0], zH[1], zH[2], zH[3], zH[4], zH[5], zH[6], zH[7]);
    // pattern=1: selects even-index elements, dst[i]=src[2i]=2i (i in [0,32)), rsvdCnt=32
    int errors = 0;
    if (rH[0] != 32) { printf("[CHECK] rsvdCnt=%u (expect 32)\n", rH[0]); errors++; }
    for (int i = 0; i < 32; i++) if (zH[i] != (float)(2 * i)) { if (errors < 5) printf("[CHECK] dst[%d]=%g (expect %d)\n", i, zH[i], 2 * i); errors++; }

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFree(rD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH)); CHECK_ACL(aclrtFreeHost(rH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());
    if (errors == 0) { printf("GATHERMASK SIMULATION PASSED\n"); return 0; }
    printf("GATHERMASK SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
