/*
 * Host launcher (single integer tensor input): launches the k_custom kernel on the simulated Ascend device.
 * dtype=int16_t (exact bit-for-bit match). x=5, verify ~x == -6 (two's complement: ~5 = -6).
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

using DT = int16_t;

int32_t main()
{
    const uint32_t blockDim = 1;
    const int32_t totalLen = 64;
    const size_t byteSize = (size_t)totalLen * sizeof(DT);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    DT *xHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, byteSize));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < totalLen; i++) { xHost[i] = (DT)(5); }
    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, byteSize, zDev, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    const DT expect = (DT)(-6);  // ~5 = -6 (two's complement)
    int errors = 0;
    for (int32_t i = 0; i < totalLen; i++) {
        if (zHost[i] != expect) {
            if (errors < 5) printf("[CHECK] idx %d = %lld (expect %lld)\n",
                                   i, (long long)zHost[i], (long long)expect);
            errors++;
        }
    }
    printf("z[0]=%lld z[last]=%lld total=%d errors=%d\n",
           (long long)zHost[0], (long long)zHost[totalLen - 1], totalLen, errors);

    if (errors == 0) printf("BITWISE_NOT SIMULATION PASSED\n");
    else             printf("BITWISE_NOT SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
