/*
 * Host: single-core launch of TransData round trip NCDHW -> NDC1HWC0 -> NCDHW.
 * NCDHW=[1,16,1,4,4] (C=16=c0, H*W=16 -> no padding), half. src[i] = i+1.
 * The round trip must reproduce src exactly over all 256 elements.
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
    const int32_t ELEM = 256;    // N*C*D*H*W = 1*16*1*4*4
    const size_t hBytes = (size_t)ELEM * sizeof(half_bits);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    half_bits *sH = nullptr, *dH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&sH, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&dH, hBytes));
    for (int i = 0; i < ELEM; i++) sH[i] = float_to_half((float)(i + 1));

    uint8_t *sD = nullptr, *dD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&sD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&dD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(sD, hBytes, sH, hBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, sD, dD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(dH, hBytes, dD, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < ELEM; i++) {
        float expect = (float)(i + 1);
        float got = half_to_float(dH[i]);
        if (fabsf(got - expect) > 0.5f) {
            if (errors < 5) printf("[CHECK] i=%d = %f (expect %f)\n", i, got, expect);
            errors++;
        }
    }
    printf("roundtrip d[0]=%f d[128]=%f d[255]=%f (expect 1,129,256) errors=%d\n",
           half_to_float(dH[0]), half_to_float(dH[128]), half_to_float(dH[255]), errors);

    if (errors == 0) printf("TRANSDATA SIMULATION PASSED\n");
    else             printf("TRANSDATA SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(sD)); CHECK_ACL(aclrtFree(dD));
    CHECK_ACL(aclrtFreeHost(sH)); CHECK_ACL(aclrtFreeHost(dH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
