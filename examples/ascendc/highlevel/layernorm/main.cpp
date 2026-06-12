/*
 * Host: single-core launch of LayerNorm. x[8,64]=3.0, gamma[64]=1.0, beta[64]=0.0, eps=1e-5.
 * Each row mean=3, var=0 -> output=(x-mean)/sqrt(eps)*gamma+beta = 0. Host verifies output ~= 0.
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
    const int32_t A = 8, R = 64, AR = A * R;
    const size_t arBytes = (size_t)AR * sizeof(float);
    const size_t rBytes = (size_t)R * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xHost = nullptr, *gHost = nullptr, *bHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *gDev = nullptr, *bDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, arBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gHost, rBytes));
    CHECK_ACL(aclrtMallocHost((void **)&bHost, rBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, arBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, arBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gDev, rBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&bDev, rBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, arBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < AR; i++) xHost[i] = 3.0f;
    for (int i = 0; i < R; i++) { gHost[i] = 1.0f; bHost[i] = 0.0f; }
    CHECK_ACL(aclrtMemcpy(xDev, arBytes, xHost, arBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gDev, rBytes, gHost, rBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(bDev, rBytes, bHost, rBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, gDev, bDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, arBytes, zDev, arBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 0.0f;
    int errors = 0;
    for (int i = 0; i < AR; i++) {
        if (fabsf(zHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, zHost[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[last]=%f expect=%f total=%d errors=%d\n",
           zHost[0], zHost[AR - 1], expect, AR, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("LAYERNORM SIMULATION PASSED\n");
    else             printf("LAYERNORM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(gDev));
    CHECK_ACL(aclrtFree(bDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(gHost));
    CHECK_ACL(aclrtFreeHost(bHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
