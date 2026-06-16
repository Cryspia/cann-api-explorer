/*
 * Host: launch the DeepNorm kernel (DeepNormTiling has its 5 shape fields hand-filled inside the kernel, no host tiling).
 * [B=1,S=1,H=8], x=1, gx=0, alpha=2, gamma=1, beta=3, eps=1e-5.
 * eff=alpha*x+gx=2 all equal -> var=0 -> dst all 3, mean=2.
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
    const int32_t H = 8;
    const size_t bytes = (size_t)H * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *gxH = nullptr, *betaH = nullptr, *gammaH = nullptr;
    float *zH = nullptr, *meanH = nullptr, *rstdH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&gxH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&betaH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&gammaH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&meanH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&rstdH, bytes));
    for (int i = 0; i < H; i++) { xH[i] = 1.0f; gxH[i] = 0.0f; betaH[i] = 3.0f; gammaH[i] = 1.0f; }
    bool golden = false;
    { const char *gx=getenv("GOLDEN_IN_X"),*gy=getenv("GOLDEN_IN_Y"),*gg=getenv("GOLDEN_IN_G"),*gb=getenv("GOLDEN_IN_B");
      if (gx) { FILE *fx=fopen(gx,"rb"); if (fx && fread(xH,sizeof(float),H,fx)==(size_t)H) golden=true; if(fx)fclose(fx); }
      if (golden&&gy){FILE*f=fopen(gy,"rb");if(f){(void)!fread(gxH,sizeof(float),H,f);fclose(f);}}
      if (golden&&gg){FILE*f=fopen(gg,"rb");if(f){(void)!fread(gammaH,sizeof(float),H,f);fclose(f);}}
      if (golden&&gb){FILE*f=fopen(gb,"rb");if(f){(void)!fread(betaH,sizeof(float),H,f);fclose(f);}} }

    uint8_t *xD = nullptr, *gxD = nullptr, *betaD = nullptr, *gammaD = nullptr;
    uint8_t *zD = nullptr, *meanD = nullptr, *rstdD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gxD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&betaD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&gammaD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&meanD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&rstdD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, bytes, xH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gxD, bytes, gxH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(betaD, bytes, betaH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(gammaD, bytes, gammaH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, gxD, betaD, gammaD, zD, meanD, rstdD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    if (golden) { const char *go=getenv("GOLDEN_OUT"); if(go){FILE*fo=fopen(go,"wb");if(fo){fwrite(zH,sizeof(float),H,fo);fclose(fo);}} }
    CHECK_ACL(aclrtMemcpy(meanH, bytes, meanD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; !golden && i < H; i++) {
        if (fabsf(zH[i] - 3.0f) > 1e-3f) {
            if (errors < 8) printf("[CHECK] dst[%d]=%g (expect 3)\n", i, zH[i]);
            errors++;
        }
    }
    if (fabsf(meanH[0] - 2.0f) > 1e-3f) { printf("[CHECK] mean=%g (expect 2)\n", meanH[0]); errors++; }
    printf("dst=[%g,%g,...,%g] mean=%g errors=%d\n", zH[0], zH[1], zH[H - 1], meanH[0], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("DEEPNORM SIMULATION PASSED\n");
    else             printf("DEEPNORM SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(gxD)); CHECK_ACL(aclrtFree(betaD));
    CHECK_ACL(aclrtFree(gammaD)); CHECK_ACL(aclrtFree(zD)); CHECK_ACL(aclrtFree(meanD)); CHECK_ACL(aclrtFree(rstdD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(gxH)); CHECK_ACL(aclrtFreeHost(betaH));
    CHECK_ACL(aclrtFreeHost(gammaH)); CHECK_ACL(aclrtFreeHost(zH)); CHECK_ACL(aclrtFreeHost(meanH));
    CHECK_ACL(aclrtFreeHost(rstdH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
