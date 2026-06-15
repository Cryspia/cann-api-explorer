/*
 * Host: single-core launch of WelfordFinalize.
 * abLength=8 partitions. inputMean all = 4.0 -> outMean = 4.0.
 * inputVariance all = 9.0, and (mean - outMean) = 0 -> outVar = 9.0. Verified on host.
 * Outputs are scalar, written at index 0 of the output buffers.
 */
#include <cstdio>
#include <cstdint>
#include <cmath>
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
    const int32_t AB = 8;
    const size_t bytes = (size_t)AB * sizeof(float);

    CHECK_ACL(aclInit(nullptr));
    CHECK_ACL(aclrtSetDevice(0));
    aclrtStream stream = nullptr;
    CHECK_ACL(aclrtCreateStream(&stream));

    float *inMH = nullptr, *inVH = nullptr, *outMH = nullptr, *outVH = nullptr;
    uint8_t *inMD = nullptr, *inVD = nullptr, *outMD = nullptr, *outVD = nullptr;
    CHECK_ACL(aclrtMallocHost((void **)&inMH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&inVH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&outMH, bytes));
    CHECK_ACL(aclrtMallocHost((void **)&outVH, bytes));
    CHECK_ACL(aclrtMalloc((void **)&inMD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&inVD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&outMD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));
    CHECK_ACL(aclrtMalloc((void **)&outVD, bytes, ACL_MEM_MALLOC_HUGE_FIRST));

    for (int i = 0; i < AB; i++) { inMH[i] = 4.0f; inVH[i] = 9.0f; outMH[i] = 0.0f; outVH[i] = 0.0f; }
    CHECK_ACL(aclrtMemcpy(inMD, bytes, inMH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));
    CHECK_ACL(aclrtMemcpy(inVD, bytes, inVH, bytes, ACL_MEMCPY_HOST_TO_DEVICE));

    ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, inMD, inVD, outMD, outVD);
    CHECK_ACL(aclrtSynchronizeStream(stream));

    CHECK_ACL(aclrtMemcpy(outMH, bytes, outMD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));
    CHECK_ACL(aclrtMemcpy(outVH, bytes, outVD, bytes, ACL_MEMCPY_DEVICE_TO_HOST));

    const float expectMean = 4.0f, expectVar = 9.0f;
    int errors = 0;
    if (fabsf(outMH[0] - expectMean) > 5e-3f) { printf("[CHECK] outMean=%f (expect %f)\n", outMH[0], expectMean); errors++; }
    if (fabsf(outVH[0] - expectVar) > 5e-3f)  { printf("[CHECK] outVar=%f (expect %f)\n", outVH[0], expectVar); errors++; }
    printf("outMean=%f outVar=%f expectMean=%f expectVar=%f errors=%d\n",
           outMH[0], outVH[0], expectMean, expectVar, errors);

    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()
    // ends the process / closes the simulator's stdout capture, so a marker printed
    // afterwards is never recorded. errors is already final here.
    if (errors == 0) printf("WELFORDFINALIZE SIMULATION PASSED\n");
    else             printf("WELFORDFINALIZE SIMULATION FAILED (%d errors)\n", errors);
    fflush(stdout);

    CHECK_ACL(aclrtFree(inMD)); CHECK_ACL(aclrtFree(inVD)); CHECK_ACL(aclrtFree(outMD)); CHECK_ACL(aclrtFree(outVD));
    CHECK_ACL(aclrtFreeHost(inMH)); CHECK_ACL(aclrtFreeHost(inVH)); CHECK_ACL(aclrtFreeHost(outMH)); CHECK_ACL(aclrtFreeHost(outVH));
    CHECK_ACL(aclrtDestroyStream(stream));
    CHECK_ACL(aclrtResetDevice(0));
    CHECK_ACL(aclFinalize());

    return errors == 0 ? 0 : 1;
}
