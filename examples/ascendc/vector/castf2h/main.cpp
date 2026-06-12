/* Host: Cast float->half, x[i]=i -> z[i]=(half)i, verified via H2F. */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

static float H2F(uint16_t h)
{
    uint16_t exp = (h >> 10) & 0x1F, mant = h & 0x3FF;
    if (exp == 0 && mant == 0) return 0.0f;
    return ldexpf(1.0f + mant / 1024.0f, (int)exp - 15);
}

int32_t main()
{
    const int32_t N = 256;
    const size_t fBytes = (size_t)N * sizeof(float);
    const size_t hBytes = (size_t)N * sizeof(uint16_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr; uint16_t *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, hBytes));
    for (int i = 0; i < N; i++) xH[i] = (float)i;

    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, hBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, fBytes, xH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, hBytes, zD, hBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) { float v = H2F(zH[i]); if (fabsf(v - (float)i) > 1e-2f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %d)\n", i, v, i); errors++; } }
    printf("z[0,1,2,255]=[%g,%g,%g,%g] errors=%d\n", H2F(zH[0]), H2F(zH[1]), H2F(zH[2]), H2F(zH[255]), errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("CASTF2H SIMULATION PASSED\n");
    else             printf("CASTF2H SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
