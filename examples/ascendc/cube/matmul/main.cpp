/*
 * Host: compute TCubeTiling (matmul_tiling API) -> copy to device -> launch the Matmul kernel.
 * A[64,64]=half all 1.0 (0x3C00), B[64,64]=half all 1.0 -> C[64,64]=float all K=64.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstring>
#include "acl/acl.h"
#include "aclrtlaunch_k_custom.h"
#include "tiling/matrix/matmul_tiling.h"   // host matmul tiling API (includes platform / kernel_tiling)

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
    const int32_t M = 64, N = 64, K = 64;
    const size_t aBytes = (size_t)M * K * sizeof(uint16_t);   // half
    const size_t bBytes = (size_t)K * N * sizeof(uint16_t);
    const size_t cBytes = (size_t)M * N * sizeof(float);
    const size_t wsBytes = 16 * 1024 * 1024;                  // workspace

    // ---- compute TCubeTiling ----
    matmul_tiling::MatmulApiTiling mmTiling;
    mmTiling.SetAType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                      matmul_tiling::DataType::DT_FLOAT16);
    mmTiling.SetBType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                      matmul_tiling::DataType::DT_FLOAT16);
    mmTiling.SetCType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                      matmul_tiling::DataType::DT_FLOAT);
    mmTiling.SetBiasType(matmul_tiling::TPosition::GM, matmul_tiling::CubeFormat::ND,
                         matmul_tiling::DataType::DT_FLOAT);
    mmTiling.SetShape(M, N, K);
    mmTiling.SetOrgShape(M, N, K);
    mmTiling.SetBias(false);
    mmTiling.SetBufferSpace(-1, -1, -1);

    AscendC::tiling::TCubeTiling tiling;
    if (mmTiling.GetTiling(tiling) == -1) {
        printf("[ERROR] GetTiling failed\n");
        return 1;
    }
    const size_t tilingBytes = sizeof(AscendC::tiling::TCubeTiling);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    uint16_t *aHost = nullptr, *bHost = nullptr;
    float *cHost = nullptr;
    uint8_t *tilingHost = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&aHost, aBytes));
    CHECK_ACL(aclrtMallocHost((void **)&bHost, bBytes));
    CHECK_ACL(aclrtMallocHost((void **)&cHost, cBytes));
    CHECK_ACL(aclrtMallocHost((void **)&tilingHost, tilingBytes));

    for (int i = 0; i < M * K; i++) aHost[i] = 0x3C00;  // half 1.0
    for (int i = 0; i < K * N; i++) bHost[i] = 0x3C00;
    memcpy(tilingHost, &tiling, tilingBytes);

    uint8_t *aDev = nullptr, *bDev = nullptr, *cDev = nullptr, *wsDev = nullptr, *tilingDev = nullptr;
    CHECK_ACL(aclrtMalloc((void **)&aDev, aBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&bDev, bBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&cDev, cBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&wsDev, wsBytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&tilingDev, tilingBytes, ACL_MEM_MALLOC_HUGE_FIRST));

    CHECK_ACL(aclrtMemcpy(aDev, aBytes, aHost, aBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(bDev, bBytes, bHost, bBytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(tilingDev, tilingBytes, tilingHost, tilingBytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(1, stream, aDev, bDev, cDev, wsDev, tilingDev);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(cHost, cBytes, cDev, cBytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expect = (float)K;   // 64
    int errors = 0;
    for (int i = 0; i < M * N; i++) {
        if (fabsf(cHost[i] - expect) > 1e-2f) {
            if (errors < 5) printf("[CHECK] idx %d = %f (expect %f)\n", i, cHost[i], expect);
            errors++;
        }
    }
    printf("C[0]=%f C[last]=%f expect=%f total=%d errors=%d\n",
           cHost[0], cHost[M * N - 1], expect, M * N, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("MATMUL SIMULATION PASSED\n");
    else             printf("MATMUL SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(aDev));
    CHECK_ACL(aclrtFree(bDev));
    CHECK_ACL(aclrtFree(cDev));
    CHECK_ACL(aclrtFree(wsDev));
    CHECK_ACL(aclrtFree(tilingDev));
    CHECK_ACL(aclrtFreeHost(aHost));
    CHECK_ACL(aclrtFreeHost(bHost));
    CHECK_ACL(aclrtFreeHost(cHost));
    CHECK_ACL(aclrtFreeHost(tilingHost));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
