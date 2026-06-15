/*
 * Host: single-core launch of the SimpleSoftMax kernel.
 * Verifiable construction:
 *   x      = all 0.0 over [M,K]=[8,64].
 *   inMax  = 0.0 per row (column 0 of each 8-float reduce block).
 *   inSum  = 64.0 per row (column 0 of each 8-float reduce block).
 *   y      = exp(x - inMax) / inSum = exp(0) / 64 = 1/64 = 0.015625 everywhere.
 * So every output element is exactly 0.015625, verified on host.
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
    const int32_t M = 8, K = 64;
    const int32_t BLK_F32 = 8;            // 32B / sizeof(float)
    const int32_t elem = M * K;           // 512
    const int32_t red = M * BLK_F32;      // 64 (reduce-tensor element count)
    const size_t byteSize = (size_t)elem * sizeof(float);
    const size_t redBytes = (size_t)red * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xHost = nullptr, *sumHost = nullptr, *maxHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *sumDev = nullptr, *maxDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&sumHost, redBytes));
    CHECK_ACL(aclrtMallocHost((void **)&maxHost, redBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, byteSize));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&sumDev, redBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&maxDev, redBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < elem; i++) xHost[i] = 0.0f;
    // reduce-layout inputs: scalar in column 0 of each row's 8-float block, rest 0.
    for (int32_t i = 0; i < red; i++) { sumHost[i] = 0.0f; maxHost[i] = 0.0f; }
    for (int32_t r = 0; r < M; r++) { sumHost[r * BLK_F32] = 64.0f; maxHost[r * BLK_F32] = 0.0f; }

    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(sumDev, redBytes, sumHost, redBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(maxDev, redBytes, maxHost, redBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, sumDev, maxDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, byteSize, zDev, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = 1.0f / (float)K;   // 0.015625
    int errors = 0;
    for (int32_t i = 0; i < elem; i++) {
        if (fabsf(zHost[i] - expect) > 1e-4f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, zHost[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[63]=%f expect=%f total=%d errors=%d\n",
           zHost[0], zHost[63], expect, elem, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SIMPLESOFTMAX SIMULATION PASSED\n");
    else             printf("SIMPLESOFTMAX SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(sumDev));
    CHECK_ACL(aclrtFree(maxDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(sumHost));
    CHECK_ACL(aclrtFreeHost(maxHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
