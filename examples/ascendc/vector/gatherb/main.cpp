/*
 * Host: Gatherb (block-level gather). src[i]=i, 64 floats = 8 blocks of 8 floats.
 * offset[k] = (8-1-k)*32 bytes selects the 8 blocks in reverse order.
 * Expect dst block b == src block (7-b): dst[8*b + j] = (7-b)*8 + j.
 */
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
    const int32_t NUM_BLK = 8;          // 8 blocks of 8 floats (32 bytes)
    const int32_t BLK_ELEMS = 8;
    const size_t fBytes = (size_t)N * sizeof(float);
    const size_t oBytes = (size_t)NUM_BLK * sizeof(uint32_t);

    CHECK_ACL(aclInit(nullptr)); CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr; CHECK_ACL(aclrtCreateStream(&stream));

    float *xH = nullptr, *zH = nullptr; uint32_t *oH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&xH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&zH, fBytes));
    CHECK_ACL(aclrtMallocHost((void **)&oH, oBytes));
    for (int i = 0; i < N; i++) xH[i] = (float)i;
    // Block byte offsets, reverse block order: block 0 of dst <- last block of src, etc.
    for (int k = 0; k < NUM_BLK; k++) oH[k] = (uint32_t)((NUM_BLK - 1 - k) * 32);

    uint8_t *xD = nullptr, *oD = nullptr, *zD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&xD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&oD, oBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&zD, fBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(xD, fBytes, xH, fBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(oD, oBytes, oH, oBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, xD, oD, zD);
    CHECK_ACL(aclrtSynchronizeStream(stream));
    CHECK_ACL(aclrtMemcpy(zH, fBytes, zD, fBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int b = 0; b < NUM_BLK; b++) {
        for (int j = 0; j < BLK_ELEMS; j++) {
            int idx = b * BLK_ELEMS + j;
            float ev = (float)((NUM_BLK - 1 - b) * BLK_ELEMS + j);
            if (zH[idx] != ev) { if (errors < 5) printf("[CHECK] z[%d]=%g (expect %g)\n", idx, zH[idx], ev); errors++; }
        }
    }
    printf("z[0..2]=[%g,%g,%g] (expect 56,57,58) z[8]=%g (expect 48) errors=%d\n",
           zH[0], zH[1], zH[2], zH[8], errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("GATHERB SIMULATION PASSED\n");
    else             printf("GATHERB SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(xD)); CHECK_ACL(aclrtFree(oD)); CHECK_ACL(aclrtFree(zD));
    CHECK_ACL(aclrtFreeHost(xH)); CHECK_ACL(aclrtFreeHost(oH)); CHECK_ACL(aclrtFreeHost(zH));
    CHECK_ACL(aclrtDestroyStream(stream)); CHECK_ACL(aclrtResetDevice(0)); CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
