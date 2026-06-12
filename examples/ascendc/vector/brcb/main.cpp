/* Host: Brcb, src[8]=[0..7] -> expect dst[b*8+k]=src[b]=b (64 elements, 8 identical per block). */
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
    const int32_t SRC_N = 8, DST_N = 64;
    const size_t sBytes = (size_t)SRC_N * sizeof(float);
    const size_t dBytes = (size_t)DST_N * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dBytes));
    for (int i = 0; i < SRC_N; i++) xH[i] = (float)i;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, sBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, sBytes, xH, sBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, dBytes, zD, dBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int b = 0; b < SRC_N; b++)
        for (int k = 0; k < 8; k++) {
            float ev = (float)b;
            if (zH[b * 8 + k] != ev) { if (errors < 5) printf("[CHECK] dst[%d]=%g (expect %g)\n", b * 8 + k, zH[b * 8 + k], ev); errors++; }
        }
    printf("dst block0=[%g,%g] block1=[%g,%g] block7=[%g] errors=%d\n", zH[0], zH[1], zH[8], zH[9], zH[56], errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());
    if (errors == 0) { printf("BRCB SIMULATION PASSED\n"); return 0; }
    printf("BRCB SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
