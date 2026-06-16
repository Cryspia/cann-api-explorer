/* Host: ShiftRight, src0[i]=16, src1[i]=2 (int32) -> expect dst[i] = 16 >> 2 = 4. */
#include <cstdlib>
#include <cstdio>
#include <cstdio>
#include <cstdint>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"

#define CHECK_ACL(x)                                                                   \
    do { aclError __ret = (x); if (__ret != ACL_SUCCESS) { printf("[ERROR] %s:%d ret=%d\n", __FILE__, __LINE__, (int)__ret); return 1; } } while (0)

int32_t main()
{
    const int32_t N = 64;
    const size_t bytes = (size_t)N * sizeof(int32_t);
    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *x0H = nullptr, *x1H = nullptr, *zH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&x0H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&x1H, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, bytes));
    bool golden=false;
    { const char*gx=getenv("GOLDEN_IN_X"),*gy=getenv("GOLDEN_IN_Y"); if(gx&&gy){ FILE*fx=fopen(gx,"rb"),*fy=fopen(gy,"rb"); if(fx&&fy&&fread(x0H,sizeof(int32_t),N,fx)==(size_t)N&&fread(x1H,sizeof(int32_t),N,fy)==(size_t)N) golden=true; if(fx)fclose(fx); if(fy)fclose(fy);} }
    if(!golden) for (int i = 0; i < N; i++) { x0H[i] = 16; x1H[i] = 2; }
    uint8_t *x0D = nullptr, *x1D = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&x0D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&x1D, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(x0D, bytes, x0H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(x1D, bytes, x1H, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, x0D, x1D, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, bytes, zD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    if(golden){ const char*go=getenv("GOLDEN_OUT"); if(go){ FILE*fo=fopen(go,"wb"); if(fo){ fwrite(zH,sizeof(int32_t),N,fo); fclose(fo); printf("[GOLDEN] dumped %d ints\n",N);} } }

    const int32_t expect = 4;
    int errors = 0;
    for (int i = 0; i < N; i++) { if (zH[i] != expect) { if (errors < 5) printf("[CHECK] z[%d]=%d (expect %d)\n", i, zH[i], expect); errors++; } }
    printf("z[0..3]=[%d,%d,%d,%d] errors=%d\n", zH[0], zH[1], zH[2], zH[3], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("SHIFTRIGHT SIMULATION PASSED\n");
    else             printf("SHIFTRIGHT SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(x0D)); CHECK_ACL(aclrtFree(x1D)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(x0H)); CHECK_ACL(aclrtFreeHost(x1H)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
