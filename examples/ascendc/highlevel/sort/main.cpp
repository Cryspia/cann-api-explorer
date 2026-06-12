/*
 * Host: launch Sort (tiling-free). src=[31..0] float, idx=[0..31], AscendC::Sort ascending.
 * Expect dst=[0..31], dstIdx=[31..0] (the original position of value i).
 */
#include <cstdio>
#include <cstdint>
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
    const int32_t N = 32;
    const size_t vBytes = (size_t)N * sizeof(float);
    const size_t iBytes = (size_t)N * sizeof(uint32_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr; uint32_t *iH = nullptr, *ziH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, vBytes));
    CHECK_ACL(aclrtMallocHost((void **)&iH, iBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, vBytes));
    CHECK_ACL(aclrtMallocHost((void **)&ziH, iBytes));
    // Diagnostic: reversed input [31..0], idx=[0..31]. Distinguishes descending/ascending/no-op.
    for (int i = 0; i < N; i++) { xH[i] = (float)(N - 1 - i); iH[i] = (uint32_t)i; }

    uint8_t *xD = nullptr, *iD = nullptr, *zD = nullptr, *ziD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, vBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&iD, iBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, vBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&ziD, iBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, vBytes, xH, vBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(iD, iBytes, iH, iBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, iD, zD, ziD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, vBytes, zD, vBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(ziH, iBytes, ziD, iBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    // Sort is ascending: input val=[31..0]/idx=[0..31] -> output val[i]=i, idx[i]=N-1-i (original position of value i).
    int errors = 0;
    for (int i = 0; i < N; i++) {
        float ev = (float)i;
        uint32_t ei = (uint32_t)(N - 1 - i);
        if (zH[i] != ev || ziH[i] != ei) {
            if (errors < 5) printf("[CHECK] [%d] val=%g idx=%u (expect %g,%u)\n", i, zH[i], ziH[i], ev, ei);
            errors++;
        }
    }
    printf("dst[0..3]=[%g,%g,%g,%g] idx[0..3]=[%u,%u,%u,%u] errors=%d\n",
           zH[0], zH[1], zH[2], zH[3], ziH[0], ziH[1], ziH[2], ziH[3], errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(iD)); CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFree(ziD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(iH)); CHECK_ACL(aclrtFreeHost(zH)); CHECK_ACL(aclrtFreeHost(ziH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    if (errors == 0) { printf("SORT SIMULATION PASSED\n"); return 0; }
    printf("SORT SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
