/*
 * Host: single-core launch of the SoftmaxFlashV3 kernel (FlashAttention-2 online softmax, first block).
 * Data types fixed by the API: src/dst = half, reduce outputs (mean/sum/max) = float.
 * Verifiable construction (isUpdate = false):
 *   x    = all 0.0 (half) over [M,K]=[8,64].
 *   With every per-row statistic = 0, the shifted input x' = 0, so:
 *     mean = 0, max = 0, y = exp(0) = 1.0 (un-normalized, NOT divided by sum), sum = K = 64.
 * Checks: every dst element == 1.0 (half); per-row reduce outputs (block layout, value in
 * col 0 of each 8-float block) mean == 0.0, max == 0.0, sum == 64.0. All verified on host.
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

// Minimal half encode/decode (sufficient for small non-negative values).
static uint16_t F2H(float f)
{
    if (f == 0.0f) return 0;
    int e = 0; float m = f;
    while (m >= 2.0f) { m /= 2.0f; e++; }
    while (m < 1.0f) { m *= 2.0f; e--; }
    uint16_t exp = (uint16_t)(e + 15);
    uint16_t mant = (uint16_t)((m - 1.0f) * 1024.0f + 0.5f);
    return (uint16_t)((exp << 10) | (mant & 0x3FF));
}
static float H2F(uint16_t h)
{
    uint16_t exp = (h >> 10) & 0x1F, mant = h & 0x3FF;
    if (exp == 0 && mant == 0) return 0.0f;
    return ldexpf(1.0f + mant / 1024.0f, (int)exp - 15);
}

int32_t main()
{
    const uint32_t blockDim = 1;
    const int32_t M = 8, K = 64;
    const int32_t BLK_F32 = 8;            // 32B / sizeof(float)
    const int32_t elem = M * K;           // 512
    const int32_t red = M * BLK_F32;      // 64 (reduce-tensor element count)
    const size_t hBytes = (size_t)elem * sizeof(uint16_t);   // half src/dst
    const size_t fBytes = (size_t)red * sizeof(float);       // float reduce outputs

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    uint16_t *xHost = nullptr, *zHost = nullptr;
    float *meanHost = nullptr, *sumHost = nullptr, *maxHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr, *meanDev = nullptr, *sumDev = nullptr, *maxDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&meanHost, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&sumHost, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&maxHost, fBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&meanDev, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&sumDev, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&maxDev, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < elem; i++) xHost[i] = F2H(0.0f);
    CHECK_ACL(aclrtMemcpy(xDev, hBytes, xHost, hBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev, meanDev, sumDev, maxDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, hBytes, zDev, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(meanHost, fBytes, meanDev, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(sumHost, fBytes, sumDev, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(maxHost, fBytes, maxDev, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    // dst: every element should be exp(0) = 1.0
    for (int32_t i = 0; i < elem; i++) {
        float v = H2F(zHost[i]);
        if (fabsf(v - 1.0f) > 1e-2f) {
            if (errors < 5) printf("[CHECK] dst idx %d = %f (expect 1.0)\n", i, v);
            errors++;
        }
    }
    // reduce outputs: value lives in column 0 of each row's 8-float block
    for (int32_t r = 0; r < M; r++) {
        float mn = meanHost[r * BLK_F32];
        float s  = sumHost[r * BLK_F32];
        float mx = maxHost[r * BLK_F32];
        if (fabsf(mn - 0.0f)  > 1e-3f) { if (errors < 10) printf("[CHECK] mean row %d = %f (expect 0.0)\n", r, mn); errors++; }
        if (fabsf(s  - 64.0f) > 1e-2f) { if (errors < 10) printf("[CHECK] sum row %d = %f (expect 64.0)\n", r, s); errors++; }
        if (fabsf(mx - 0.0f)  > 1e-3f) { if (errors < 10) printf("[CHECK] max row %d = %f (expect 0.0)\n", r, mx); errors++; }
    }
    printf("dst[0]=%f mean[row0]=%f sum[row0]=%f max[row0]=%f errors=%d\n",
           H2F(zHost[0]), meanHost[0], sumHost[0], maxHost[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SOFTMAXFLASHV3 SIMULATION PASSED\n");
    else             printf("SOFTMAXFLASHV3 SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFree(meanDev));
    CHECK_ACL(aclrtFree(sumDev));
    CHECK_ACL(aclrtFree(maxDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtFreeHost(meanHost));
    CHECK_ACL(aclrtFreeHost(sumHost));
    CHECK_ACL(aclrtFreeHost(maxHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
