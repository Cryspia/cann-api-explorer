/* Host: CastDequant. deqScale=2.0. src(int32)=3 -> dst = 3*2 = 6 (half). */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

static float half_to_float(uint16_t h)
{
    uint32_t sign = (uint32_t)(h >> 15) & 0x1u;
    uint32_t exp  = (uint32_t)(h >> 10) & 0x1Fu;
    uint32_t mant = (uint32_t)h & 0x3FFu;
    uint32_t f;
    if (exp == 0) {
        if (mant == 0) { f = sign << 31; }
        else { exp = 127 - 15 + 1; while ((mant & 0x400u) == 0) { mant <<= 1; exp--; } mant &= 0x3FFu;
               f = (sign << 31) | (exp << 23) | (mant << 13); }
    } else if (exp == 0x1Fu) { f = (sign << 31) | (0xFFu << 23) | (mant << 13); }
    else { f = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13); }
    float out; __builtin_memcpy(&out, &f, sizeof(out)); return out;
}

int32_t main()
{
    const int32_t N = 256;
    const size_t sBytes = (size_t)N * sizeof(int32_t);
    const size_t dBytes = (size_t)N * sizeof(uint16_t);

    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *xH = nullptr; uint16_t *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dBytes));
    for (int i = 0; i < N; i++) xH[i] = 3;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, sBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, dBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, sBytes, xH, sBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, dBytes, zD, dBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) {
        float got = half_to_float(zH[i]);
        if (fabsf(got - 6.0f) > 1e-2f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect 6)\n", i, got); errors++; }
    }
    printf("z[0]=%g z[last]=%g (expect 6) errors=%d\n", half_to_float(zH[0]), half_to_float(zH[N - 1]), errors);

    if (errors == 0) printf("CASTDEQUANT SIMULATION PASSED\n");
    else             printf("CASTDEQUANT SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
