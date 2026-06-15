/*
 * Host: single-core launch of the SoftmaxGradFront kernel.
 * Verifiable construction:
 *   x    = forward softmax output, taken uniform = 1/K = 1/64 (each row sums to 1).
 *   grad = constant c = 2.0 in every element.
 *   y    = rowsum(grad * x) = K * (c / K) = c = 2.0  (one scalar per row).
 * The output is reduce-shaped: each row's scalar lives in column 0 of its own 8-float
 * block, so we read y[row * 8]. Every row should be exactly 2.0, verified on host.
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

    float *gradHost = nullptr, *xHost = nullptr, *zHost = nullptr;
    uint8_t *gradDev = nullptr, *xDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&gradHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, redBytes));
    CHECK_ACL(aclrtMalloc((void **)&gradDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, redBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    const float c = 2.0f;
    for (int32_t i = 0; i < elem; i++) { gradHost[i] = c; xHost[i] = 1.0f / (float)K; }
    CHECK_ACL(aclrtMemcpy(gradDev, byteSize, gradHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, gradDev, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, redBytes, zDev, redBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = c;   // rowsum(grad * x) = 2.0
    int errors = 0;
    for (int32_t r = 0; r < M; r++) {
        float v = zHost[r * BLK_F32];
        if (fabsf(v - expect) > 1e-3f) {
            if (errors < 5) printf("[CHECK] row %d = %f (expect %f)\n", r, v, expect);
            errors++;
        }
    }
    printf("y[row0]=%f y[row7]=%f expect=%f rows=%d errors=%d\n",
           zHost[0], zHost[(M - 1) * BLK_F32], expect, M, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SOFTMAXGRADFRONT SIMULATION PASSED\n");
    else             printf("SOFTMAXGRADFRONT SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(gradDev));
    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(gradHost));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
