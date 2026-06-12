/*
 * Auto-generated host launcher (cast): source float filled with 3.0,
 * converted to int32_t by k_custom, host-side verify == 3 (exact integer match).
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
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
using DT = int32_t;

int32_t main()
{
    const uint32_t blockDim = 8;
    const int32_t totalLen = 8 * 2048;
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

    for (int32_t i = 0; i < totalLen; i++) xHost[i] = (ST)(3.0);
    CHECK_ACL(aclrtMemcpy(xDev, srcBytes, xHost, srcBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, dstBytes, zDev, dstBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const DT expect = (DT)(3);
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

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("CAST SIMULATION PASSED\n");
    else             printf("CAST SIMULATION FAILED (%d errors)\n", errors);
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
