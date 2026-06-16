/*
 * Auto-generated host launcher (reduction): single-core launch of k_custom, reduces 256 copies of 1.0,
 * host-side verify dst[0] ~ 256.0. dtype=float.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>   // getenv (golden bridge)
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

int32_t main()
{
    const uint32_t blockDim = 1;          // reduction uses a single core
    const int32_t elem = 256;
    const size_t byteSize = (size_t)elem * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, byteSize));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, byteSize));
    CHECK_ACL(aclrtMalloc((void **)&xDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, byteSize, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int32_t i = 0; i < elem; i++) xHost[i] = (float)(1.0);
    bool golden = false;
    { const char *gx = getenv("GOLDEN_IN_X");
      if (gx) { FILE *fx = fopen(gx, "rb");
        if (fx && fread(xHost, sizeof(float), elem, fx) == (size_t)elem) golden = true;
        if (fx) fclose(fx); } }
    CHECK_ACL(aclrtMemcpy(xDev, byteSize, xHost, byteSize, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, byteSize, zDev, byteSize, ACL_MEMCPY_DEVICE_TO_HOST));

    if (golden) { const char *go = getenv("GOLDEN_OUT");
      if (go) { FILE *fo = fopen(go, "wb"); if (fo) { fwrite(zHost, sizeof(float), 1, fo); fclose(fo); printf("[GOLDEN] dumped 1 float to %s\n", go); } } }

    const float expect = (float)(256.0);
    int errors = 0;
    float v = zHost[0];                   // the reduction result is in dst[0]
    if (!golden && fabsf(v - expect) > 1e-3f) {
        printf("[CHECK] dst[0] = %f (expect %f)\n", v, expect);
        errors++;
    }
    printf("dst[0]=%f expect=%f errors=%d\n", v, expect, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("REDUCESUM SIMULATION PASSED\n");
    else             printf("REDUCESUM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
