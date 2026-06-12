/* Host: Add half variant, a=1.0 b=2.0 -> c=3.0 (half encode/decode F2H/H2F). */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

// Minimal half encode/decode (sufficient for small non-negative integers)
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
    const int32_t N = 256;
    const size_t bytes = (size_t)N * sizeof(uint16_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    uint16_t *xH = nullptr, *yH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&yH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    for (int i = 0; i < N; i++) { xH[i] = F2H(1.0f); yH[i] = F2H(2.0f); }

    uint8_t *xD = nullptr, *yD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&yD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(yD, bytes, yH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, yD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) { float v = H2F(zH[i]); if (fabsf(v - 3.0f) > 1e-2f) { if (errors < 5) printf("[CHECK] z[%d]=%g\n", i, v); errors++; } }
    printf("z[0..2]=[%g,%g,%g] (half 1+2=3) errors=%d\n", H2F(zH[0]), H2F(zH[1]), H2F(zH[2]), errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("ADD_HALF SIMULATION PASSED\n");
    else             printf("ADD_HALF SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(yD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(yH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
