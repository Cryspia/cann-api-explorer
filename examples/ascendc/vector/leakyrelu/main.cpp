/* Host: LeakyRelu, negSlope=0.1; src=-2 (even idx) -> -0.2, src=3 (odd idx) -> 3. */
#include <cstdlib>
#include <cstdio>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const float NEG_SLOPE = 0.1f;
    const size_t bytes = (size_t)N * sizeof(float);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    bool golden=false;
    { const char*gx=getenv("GOLDEN_IN_X"); if(gx){ FILE*fx=fopen(gx,"rb"); if(fx&&fread(xH,sizeof(float),N,fx)==(size_t)N) golden=true; if(fx)fclose(fx);} }
    if(!golden) for (int i = 0; i < N; i++) xH[i] = (i % 2 == 0) ? -2.0f : 3.0f;
    uint8_t *xD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    if(golden){ const char*go=getenv("GOLDEN_OUT"); if(go){ FILE*fo=fopen(go,"wb"); if(fo){ fwrite(zH,sizeof(float),N,fo); fclose(fo); printf("[GOLDEN] dumped %d floats\n",(int)N);} } }

    int errors = 0;
    for (int i = 0; i < N; i++) {
        float ev = (xH[i] >= 0.0f) ? xH[i] : NEG_SLOPE * xH[i];
        if (fabsf(zH[i] - ev) > 5e-3f) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %g)\n", i, zH[i], ev); errors++; }
    }
    printf("z[0..3]=[%g,%g,%g,%g] errors=%d\n", zH[0], zH[1], zH[2], zH[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("LEAKYRELU SIMULATION PASSED\n");
    else             printf("LEAKYRELU SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
