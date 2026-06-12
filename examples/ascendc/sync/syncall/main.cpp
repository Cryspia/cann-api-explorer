/*
 * Host: launches the multi-core SyncAll kernel (blockDim=8).
 * Each core writes idx+1 into the idx-th 32B slot (SLOT=8 int32); after SyncAll each core reads all slots and sums.
 * Expect flag[i*SLOT]=i+1, result[i*SLOT]=1+2+..+8=36 (all cores).
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
    const size_t bytes = (size_t)N * SLOT * sizeof(int32_t);   // 32B slot per core
    const size_t syncBytes = (size_t)N * sizeof(int32_t);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    int32_t *flagH = nullptr, *resH = nullptr, *syncH = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&flagH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&resH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&syncH, syncBytes));
    for (int i = 0; i < N * SLOT; i++) { flagH[i] = -1; resH[i] = 0; }
    for (int i = 0; i < N; i++) syncH[i] = 0;

    uint8_t *flagD = nullptr, *resD = nullptr, *syncD = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&flagD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&resD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&syncD, syncBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMemcpy(flagD, bytes, flagH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(syncD, syncBytes, syncH, syncBytes, ACL_MEMCPY_HOST_TO_DEVICE));  // zero the barrier workspace

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, flagD, resD, syncD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(flagH, bytes, flagD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(resH, bytes, resD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const int32_t expectSum = N * (N + 1) / 2;   // 36
    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (flagH[i * SLOT] != i + 1) { printf("[CHECK] flag[%d]=%d (expect %d)\n", i, flagH[i * SLOT], i + 1); errors++; }
        if (resH[i * SLOT] != expectSum) { printf("[CHECK] result[%d]=%d (expect %d)\n", i, resH[i * SLOT], expectSum); errors++; }
    }
    printf("flag=[%d,%d,..,%d] result=[%d,%d,..,%d] expectSum=%d errors=%d\n",
           flagH[0], flagH[SLOT], flagH[(N - 1) * SLOT],
           resH[0], resH[SLOT], resH[(N - 1) * SLOT], expectSum, errors);

    CHECK_ACL(aclrtFree(flagD)); CHECK_ACL(aclrtFree(resD)); CHECK_ACL(aclrtFree(syncD));
    CHECK_ACL(aclrtFreeHost(flagH)); CHECK_ACL(aclrtFreeHost(resH)); CHECK_ACL(aclrtFreeHost(syncH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    if (errors == 0) { printf("SYNCALL SIMULATION PASSED\n"); return 0; }
    printf("SYNCALL SIMULATION FAILED (%d errors)\n", errors);
    return 1;
}
