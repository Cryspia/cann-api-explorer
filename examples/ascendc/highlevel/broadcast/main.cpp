/*
 * Host: launch the Broadcast kernel (no host tiling; computed device-side by GetBroadcastTilingInfo).
 * src=[0..7] float ([1,8]), broadcast to dst [4,8]: every row is 0..7.
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
    const int32_t M = 4, N = 8;
    const size_t srcBytes = (size_t)N * sizeof(float);
    const size_t dstBytes = (size_t)M * N * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, srcBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dstBytes));
    for (int j = 0; j < N; j++) xH[j] = (float)j;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, srcBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dstBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, srcBytes, xH, srcBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, dstBytes, zD, dstBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float ev = (float)j;
            if (zH[i * N + j] != ev) {
                if (errors < 8) printf("[CHECK] dst[%d][%d]=%g (expect %g)\n", i, j, zH[i * N + j], ev);
                errors++;
            }
        }
    }
    printf("dst row0=[%g..%g] row3=[%g..%g] errors=%d\n",
           zH[0], zH[N - 1], zH[3 * N], zH[3 * N + N - 1], errors);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    if (errors == 0) { printf("BROADCAST SIMULATION PASSED\n"); return 0; }
    printf("BROADCAST SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
