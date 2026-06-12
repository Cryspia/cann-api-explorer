/*
 * Host: launches the Pad kernel (no host tiling, the 3 fields are hand-filled inside the kernel).
 * src=[1..16] float, srcOriWidth=12, rightPad=4, padValue=7.
 * Expect dst[0..11]=src[0..11] (i.e. 1..12), dst[12..15]=7.
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
    const int32_t W = 16, ORI_W = 12, RIGHT_PAD = 4;
    const float PAD_VALUE = 7.0f;
    const size_t bytes = (size_t)W * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < W; i++) xH[i] = (float)(i + 1);

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < W; i++) {
        float ev = (i < ORI_W) ? (float)(i + 1) : PAD_VALUE;
        if (zH[i] != ev) {
            if (errors < 8) printf("[CHECK] dst[%d]=%g (expect %g)\n", i, zH[i], ev);
            errors++;
        }
    }
    printf("dst=[");
    for (int i = 0; i < W; i++) printf("%g%s", zH[i], i < W - 1 ? "," : "");
    printf("] errors=%d\n", errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    (void)RIGHT_PAD;
    if (errors == 0) { printf("PAD SIMULATION PASSED\n"); return 0; }
    printf("PAD SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
