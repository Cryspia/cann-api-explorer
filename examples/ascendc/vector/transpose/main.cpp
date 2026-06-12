/*
 * Host: launch Transpose (16x16 half block). src[r][c]=r*16+c (0..255, exact in half),
 * expect dst[r][c]=src[c][r]=c*16+r. host verifies element by element.
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

// Minimal half encode/decode (only small non-negative integers, sufficient for 0..255)
static uint16_t F2H(float f)
{
    if (f == 0.0f) return 0;
    int e = 15; float m = f;
    while (m >= 2.0f) { m /= 2.0f; e++; }
    while (m < 1.0f)  { m *= 2.0f; e--; }
    uint16_t mant = (uint16_t)((m - 1.0f) * 1024.0f + 0.5f);
    return (uint16_t)((e << 10) | (mant & 0x3FF));
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
    const int32_t H = 16, W = 16, N = H * W;
    const size_t byteSize = (size_t)N * sizeof(uint16_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    uint16_t *xHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, byteSize));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int r = 0; r < H; r++)
        for (int c = 0; c < W; c++)
            xHost[r * W + c] = F2H((float)(r * W + c));
    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, byteSize, zDev, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            float v = H2F(zHost[r * W + c]);
            float expect = (float)(c * W + r);   // dst[r][c] = src[c][r]
            if (fabsf(v - expect) > 0.5f) {
                if (errors < 5) printf("[CHECK] dst[%d][%d]=%f (expect %f)\n", r, c, v, expect);
                errors++;
            }
        }
    }
    printf("dst[0][1]=%f (expect 16) dst[1][0]=%f (expect 1) errors=%d\n",
           H2F(zHost[0 * W + 1]), H2F(zHost[1 * W + 0]), errors);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    if (errors == 0) {
        printf("TRANSPOSE SIMULATION PASSED\n");
        return 0;
    }
    printf("TRANSPOSE SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
