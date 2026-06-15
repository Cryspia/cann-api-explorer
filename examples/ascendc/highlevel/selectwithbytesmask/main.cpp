/*
 * Host: single-core launch of SelectWithBytesMask.
 * Shape [16,16], T=half. src0[i]=i, src1 scalar=99. Per row: first 8 cols mask=0 (keep src0=index),
 * last 8 cols mask=1 (take scalar 99). Host rebuilds the expected result element-by-element.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

typedef unsigned short half_bits;

// Minimal half<->float helpers (round-to-nearest, no subnormal handling needed for small integers).
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
    const int32_t FIRST = 16, LAST = 16, ELEM = FIRST * LAST;
    const size_t hBytes = (size_t)ELEM * sizeof(half_bits);
    const size_t mBytes = (size_t)ELEM * sizeof(uint8_t);
    const float scalar = 99.0f;

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    half_bits *s0H = nullptr, *dH = nullptr; uint8_t *mH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&s0H, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&dH, hBytes));
    CHECK_ACL(aclrtMallocHost((void **)&mH, mBytes));
    for (int r = 0; r < FIRST; r++) {
        for (int c = 0; c < LAST; c++) {
            int idx = r * LAST + c;
            s0H[idx] = float_to_half((float)idx);
            mH[idx] = (c < 8) ? 0 : 1;   // 0 -> keep src0, 1 -> take scalar
        }
    }

    uint8_t *s0D = nullptr, *mD = nullptr, *dD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&s0D, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&mD, mBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&dD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(s0D, hBytes, s0H, hBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(mD, mBytes, mH, mBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, s0D, mD, dD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(dH, hBytes, dD, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int r = 0; r < FIRST; r++) {
        for (int c = 0; c < LAST; c++) {
            int idx = r * LAST + c;
            float expect = (c < 8) ? (float)idx : scalar;
            float got = half_to_float(dH[idx]);
            if (fabsf(got - expect) > 0.5f) {
                if (errors < 5) printf("[CHECK] idx %d (r=%d,c=%d) = %f (expect %f)\n", idx, r, c, got, expect);
                errors++;
            }
        }
    }
    printf("d[0]=%f d[8]=%f d[15]=%f (expect 0,99,99) errors=%d\n",
           half_to_float(dH[0]), half_to_float(dH[8]), half_to_float(dH[15]), errors);

    if (errors == 0) printf("SELECTWITHBYTESMASK SIMULATION PASSED\n");
    else             printf("SELECTWITHBYTESMASK SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(s0D)); CHECK_ACL(aclrtFree(mD)); CHECK_ACL(aclrtFree(dD));
    CHECK_ACL(aclrtFreeHost(s0H)); CHECK_ACL(aclrtFreeHost(mH)); CHECK_ACL(aclrtFreeHost(dH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
