/*
 * Host: single-core launch of ConfusionTranspose (TRANSPOSE_ND2ND_021, dim0=1 -> plain 2D transpose).
 * src[h][w] = h*16 + w ; expect dst[i][j] = j*16 + i. Host verifies the full transposed matrix.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

typedef unsigned short half_bits;

static float half_to_float(half_bits h)
{
    uint32_t sign = (h >> 15) & 0x1;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;
    uint32_t f;
    if (exp == 0) {
        if (mant == 0) { f = sign << 31; }
        else {
            exp = 127 - 15 + 1;
            while ((mant & 0x400) == 0) { mant <<= 1; exp--; }
            mant &= 0x3FF;
            f = (sign << 31) | (exp << 23) | (mant << 13);
        }
    } else if (exp == 0x1F) {
        f = (sign << 31) | (0xFF << 23) | (mant << 13);
    } else {
        f = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13);
    }
    float out; __builtin_memcpy(&out, &f, sizeof(out)); return out;
}
static half_bits float_to_half(float v)
{
    uint32_t x; __builtin_memcpy(&x, &v, sizeof(x));
    uint32_t sign = (x >> 16) & 0x8000;
    int32_t exp = ((x >> 23) & 0xFF) - 127 + 15;
    uint32_t mant = x & 0x7FFFFF;
    if (exp <= 0) return (half_bits)sign;
    if (exp >= 0x1F) return (half_bits)(sign | 0x7C00);
    return (half_bits)(sign | (exp << 10) | (mant >> 13));
}

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
    const int32_t H = 16, W = 16, ELEM = H * W;
    const size_t hBytes = (size_t)ELEM * sizeof(half_bits);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    half_bits *sH = nullptr, *dH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&sH, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&dH, hBytes));
    for (int h = 0; h < H; h++)
        for (int w = 0; w < W; w++)
            sH[h * W + w] = float_to_half((float)(h * W + w));

    uint8_t *sD = nullptr, *dD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&sD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&dD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(sD, hBytes, sH, hBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, sD, dD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(dH, hBytes, dD, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < H; i++) {        // dst row index (= src col)
        for (int j = 0; j < W; j++) {    // dst col index (= src row)
            float expect = (float)(j * W + i);   // dst[i][j] = src[j][i] = j*W + i
            float got = half_to_float(dH[i * W + j]);
            if (fabsf(got - expect) > 0.5f) {
                if (errors < 5) printf("[CHECK] dst[%d][%d]=%f (expect %f)\n", i, j, got, expect);
                errors++;
            }
        }
    }
    printf("dst[0][1]=%f (expect 16) dst[1][0]=%f (expect 1) dst[15][15]=%f (expect 255) errors=%d\n",
           half_to_float(dH[0 * W + 1]), half_to_float(dH[1 * W + 0]), half_to_float(dH[15 * W + 15]), errors);

    if (errors == 0) printf("CONFUSIONTRANSPOSE SIMULATION PASSED\n");
    else             printf("CONFUSIONTRANSPOSE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(sD)); CHECK_ACL(aclrtFree(dD));
    CHECK_ACL(aclrtFreeHost(sH)); CHECK_ACL(aclrtFreeHost(dH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
