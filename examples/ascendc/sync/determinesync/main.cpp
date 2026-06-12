/*
 * Host: launches the chained IBSet/IBWait kernel (blockDim=8).
 * Core i waits for core i-1 to finish, then writes chain[i]=chain[i-1]+1 (core 0 writes 1). Expect chain[i*SLOT]=i+1 (core 7=8).
 */
#include <cstdio>
#include <cstdint>
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
    const int32_t N = 8, SLOT = 8;
    const uint32_t blockDim = 8;
    const size_t bytes = (size_t)64 * sizeof(int32_t);   // chain/sync each 64 int32

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *chainH = nullptr, *syncH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&chainH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&syncH, bytes));
    for (int i = 0; i < 64; i++) { chainH[i] = -1; syncH[i] = 0; }

    uint8_t *chainD = nullptr, *syncD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&chainD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&syncD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(chainD, bytes, chainH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(syncD, bytes, syncH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));  // zero the sync slots

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, chainD, syncD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(chainH, bytes, chainD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (chainH[i * SLOT] != i + 1) {
            printf("[CHECK] chain[%d]=%d (expect %d)\n", i, chainH[i * SLOT], i + 1);
            errors++;
        }
    }
    printf("chain=[%d,%d,%d,..,%d] expect[0,1,2,..,%d] errors=%d\n",
           chainH[0], chainH[SLOT], chainH[2 * SLOT], chainH[(N - 1) * SLOT], N, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("DETERMINESYNC SIMULATION PASSED\n");
    else             printf("DETERMINESYNC SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(chainD)); CHECK_ACL(aclrtFree(syncD));
    CHECK_ACL(aclrtFreeHost(chainH)); CHECK_ACL(aclrtFreeHost(syncH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
