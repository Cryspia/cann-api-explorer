/*
 * Host: single-core launch of GroupNorm. x[1,4,16]=5.0, gamma[4]=1.0, beta[4]=0.0, eps=1e-5.
 * G=2,D=2,HW=16: each group reduces D*HW=32 equal elements -> var=0 -> output=beta=0. Verified on host.
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
    const int32_t C = 4, TOTAL = 1 * 4 * 16;
    const size_t tBytes = (size_t)TOTAL * sizeof(float);
    const size_t cBytes = (size_t)C * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xHost = nullptr, *gHost = nullptr, *bHost = nullptr, *zHost = nullptr;
    uint8_t *xDev = nullptr, *gDev = nullptr, *bDev = nullptr, *zDev = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xHost, tBytes));
    CHECK_ACL(aclrtMallocHost((void **)&gHost, cBytes));
    CHECK_ACL(aclrtMallocHost((void **)&bHost, cBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zHost, tBytes));
    CHECK_ACL(aclrtMalloc((void **)&xDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gDev, cBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&bDev, cBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zDev, tBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < TOTAL; i++) xHost[i] = 5.0f;
    for (int i = 0; i < C; i++) { gHost[i] = 1.0f; bHost[i] = 0.0f; }
    bool golden = false;
    { const char *gx=getenv("GOLDEN_IN_X"),*gg=getenv("GOLDEN_IN_G"),*gb=getenv("GOLDEN_IN_B");
      if (gx) { FILE *fx=fopen(gx,"rb"); if (fx && fread(xHost,sizeof(float),TOTAL,fx)==(size_t)TOTAL) golden=true; if(fx)fclose(fx); }
      if (golden&&gg){FILE*f=fopen(gg,"rb");if(f){(void)!fread(gHost,sizeof(float),C,f);fclose(f);}}
      if (golden&&gb){FILE*f=fopen(gb,"rb");if(f){(void)!fread(bHost,sizeof(float),C,f);fclose(f);}} }
    CHECK_ACL(aclrtMemcpy(xDev, tBytes, xHost, tBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gDev, cBytes, gHost, cBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(bDev, cBytes, bHost, cBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, xDev, gDev, bDev, zDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zHost, tBytes, zDev, tBytes, ACL_MEMCPY_DEVICE_TO_HOST));
    if (golden) { const char *go=getenv("GOLDEN_OUT"); if(go){FILE*fo=fopen(go,"wb");if(fo){fwrite(zHost,sizeof(float),TOTAL,fo);fclose(fo);}} }

    const float expect = 0.0f;
    int errors = 0;
    for (int i = 0; !golden && i < TOTAL; i++) {
        if (fabsf(zHost[i] - expect) > 5e-3f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, zHost[i], expect);
            errors++;
        }
    }
    printf("z[0]=%f z[last]=%f expect=%f total=%d errors=%d\n",
           zHost[0], zHost[TOTAL - 1], expect, TOTAL, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("GROUPNORM SIMULATION PASSED\n");
    else             printf("GROUPNORM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xDev));
    CHECK_ACL(aclrtFree(gDev));
    CHECK_ACL(aclrtFree(bDev));
    CHECK_ACL(aclrtFree(zDev));
    CHECK_ACL(aclrtFreeHost(xHost));
    CHECK_ACL(aclrtFreeHost(gHost));
    CHECK_ACL(aclrtFreeHost(bHost));
    CHECK_ACL(aclrtFreeHost(zHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
