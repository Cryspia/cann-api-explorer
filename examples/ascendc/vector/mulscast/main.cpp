/* Host: MulsCast, src0[i]=i (float), scalar=0.5 -> expect dst[i]=half(0.5*i). */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

// Minimal IEEE half -> float decode for host-side verification.
static float half_to_float(uint16_t h)
{
    uint32_t sign = (uint32_t)(h >> 15) & 0x1u;
    uint32_t exp  = (uint32_t)(h >> 10) & 0x1Fu;
    uint32_t mant = (uint32_t)h & 0x3FFu;
    uint32_t f;
    if (exp == 0) {
        if (mant == 0) { f = sign << 31; }
        else {
            exp = 127 - 15 + 1;
            while ((mant & 0x400u) == 0) { mant <<= 1; exp--; }
            mant &= 0x3FFu;
            f = (sign << 31) | (exp << 23) | (mant << 13);
        }
    } else if (exp == 0x1Fu) {
        f = (sign << 31) | (0xFFu << 23) | (mant << 13);
    } else {
        f = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13);
    }
    float out; __builtin_memcpy(&out, &f, sizeof(out)); return out;
}

int32_t main()
{
    const int32_t N = 64;
    const float SCALAR = 0.5f;
    const size_t sBytes = (size_t)N * sizeof(float);
    const size_t dBytes = (size_t)N * sizeof(uint16_t);   // half on host
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr; uint16_t *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, sBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, dBytes));
    for (int i = 0; i < N; i++) xH[i] = (float)i;
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
        float ev  = SCALAR * (float)i;
        if (fabsf(got - ev) > 5e-3f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %g)\n", i, got, ev); errors++; }
    }
    printf("z[0..3]=[%g,%g,%g,%g] errors=%d\n",
           half_to_float(zH[0]), half_to_float(zH[1]), half_to_float(zH[2]), half_to_float(zH[3]), errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("MULSCAST SIMULATION PASSED\n");
    else             printf("MULSCAST SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
