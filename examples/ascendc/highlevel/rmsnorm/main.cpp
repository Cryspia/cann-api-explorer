/*
 * Host: single-core launch of RmsNorm. x[1,8,64] all 2.0, gamma[64] all 1.0, eps=1e-5.
 * rms = sqrt(mean(x^2)+eps) ~= 2.0 -> y = x/rms*gamma ~= 1.0. Verified on host.
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

int32_t main()
{
    const uint32_t blockDim = 1;
    const int32_t bsh = 1 * 8 * 64;
    const int32_t h = 64;
    const size_t xBytes = (size_t)bsh * sizeof(float);
    const size_t gBytes = (size_t)h * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xHost = nullptr, *gHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *gDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, xBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gHost, gBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, xBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, xBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gDev, gBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, xBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < bsh; i++) xHost[i] = 2.0f;
    for (int32_t i = 0; i < h; i++) gHost[i] = 1.0f;
    CHECK_ACL(aclrtMemcpy(xDev, xBytes, xHost, xBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gDev, gBytes, gHost, gBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, gDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, xBytes, zDev, xBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 1.0f;
    int errors = 0;
    for (int32_t i = 0; i < bsh; i++) {
        if (fabsf(zHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, zHost[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[last]=%f expect=%f total=%d errors=%d\n",
           zHost[0], zHost[bsh - 1], expect, bsh, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("RMSNORM SIMULATION PASSED\n");
    else             printf("RMSNORM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(gDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(gHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
