/*
 * Host launcher for AscendQuant: source float filled with 2.0, quantized by k_custom
 * with scale=1.0, offset=0.0 -> dst(int8) = round(2.0 * 1.0 + 0.0) = 2.
 * Output is int8; verify == 2 (exact integer match).
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

using ST = float;
using DT = int8_t;

int32_t main()
{
    const uint32_t blockDim = 1;
    const int32_t totalLen = 256;
    const size_t srcBytes = (size_t)totalLen * sizeof(ST);
    const size_t dstBytes = (size_t)totalLen * sizeof(DT);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    ST *xHost = nullptr; DT *zHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, srcBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, dstBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, srcBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, dstBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < totalLen; i++) xHost[i] = (ST)(2.0);
    CHECK_ACL(aclrtMemcpy(xDev, srcBytes, xHost, srcBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, dstBytes, zDev, dstBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const DT expect = (DT)(2);
    int errors = 0;
    for (int32_t i = 0; i < totalLen; i++) {
        if (zHost[i] != expect) {
            if (errors < 5) printf("[CHECK] idx %d = %d (expect %d)\n",
                                   i, (int)zHost[i], (int)expect);
            errors++;
        }
    }
    printf("z[0]=%d z[last]=%d total=%d errors=%d\n",
           (int)zHost[0], (int)zHost[totalLen - 1], totalLen, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: aclFinalize() may end the
    // process / close the simulator's stdout capture on some hosts.
    if (errors == 0) printf("ASCENDQUANT SIMULATION PASSED\n");
    else             printf("ASCENDQUANT SIMULATION FAILED (%d errors)\n", errors);
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
