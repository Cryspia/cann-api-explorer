/*
 * Host: single-core launch of the SoftMax kernel. Input [8,64] all filled with 1.0,
 * softmax along the last dimension K=64 -> each element = 1/64 = 0.015625, verified on host.
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
    const uint32_t blockDim = 1;
    const int32_t elem = 8 * 64;
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

    for (int32_t i = 0; i < elem; i++) xHost[i] = 1.0f;
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
      if (go) { FILE *fo = fopen(go, "wb"); if (fo) { fwrite(zHost, sizeof(float), elem, fo); fclose(fo); printf("[GOLDEN] dumped %d floats to %s\n", elem, go); } } }

    const float expect = 1.0f / 64.0f;       // 0.015625
    int errors = 0;
    for (int32_t i = 0; !golden && i < elem; i++) {
        if (fabsf(zHost[i] - expect) > 1e-3f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, zHost[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[63]=%f expect=%f total=%d errors=%d\n",
           zHost[0], zHost[63], expect, elem, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SOFTMAX SIMULATION PASSED\n");
    else             printf("SOFTMAX SIMULATION FAILED (%d errors)\n", errors);
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
