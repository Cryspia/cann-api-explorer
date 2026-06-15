/*
 * Host: single-core launch of the SoftmaxFlashV2 kernel (FlashAttention-2 online softmax, first block).
 * Verifiable construction (isUpdate = false):
 *   x   = all 0.0 over [M,K]=[8,64].
 *   max = rowmax(x) = 0.
 *   y   = exp(x - max) = exp(0) = 1.0   (un-normalized, NOT divided by sum).
 *   sum = rowsum(y)   = K = 64.
 * Checks: every dst element == 1.0; per-row reduce outputs (block layout, value in col 0 of each
 * 8-float block) sum == 64.0 and max == 0.0. All verified on host.
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

    float *xHost = nullptr, *zHost = nullptr, *sumHost = nullptr, *maxHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr, *sumDev = nullptr, *maxDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&sumHost, redBytes));
    CHECK_ACL(aclrtMallocHost((void **)&maxHost, redBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&sumDev, redBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&maxDev, redBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < elem; i++) xHost[i] = 0.0f;
    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev, sumDev, maxDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, byteSize, zDev, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(sumHost, redBytes, sumDev, redBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(maxHost, redBytes, maxDev, redBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    // dst: every element should be exp(0) = 1.0
    for (int32_t i = 0; i < elem; i++) {
        if (fabsf(zHost[i] - 1.0f) > 1e-3f) {
            if (errors < 5) printf("[CHECK] dst idx %d = %f (expect 1.0)\n", i, zHost[i]);
            errors++;
        }
    }
    // reduce outputs: value lives in column 0 of each row's 8-float block
    for (int32_t r = 0; r < M; r++) {
        float s = sumHost[r * BLK_F32];
        float mx = maxHost[r * BLK_F32];
        if (fabsf(s - 64.0f) > 1e-3f) { if (errors < 10) printf("[CHECK] sum row %d = %f (expect 64.0)\n", r, s); errors++; }
        if (fabsf(mx - 0.0f) > 1e-3f) { if (errors < 10) printf("[CHECK] max row %d = %f (expect 0.0)\n", r, mx); errors++; }
    }
    printf("dst[0]=%f sum[row0]=%f max[row0]=%f errors=%d\n",
           zHost[0], sumHost[0], maxHost[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SOFTMAXFLASHV2 SIMULATION PASSED\n");
    else             printf("SOFTMAXFLASHV2 SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFree(sumDev));
    CHECK_ACL(aclrtFree(maxDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtFreeHost(sumHost));
    CHECK_ACL(aclrtFreeHost(maxHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
