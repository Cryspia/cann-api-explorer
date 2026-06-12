/*
 * Hand-written advanced sample: Matmul (Cube, with TCubeTiling).
 * C[M,N] = A[M,K] @ B[K,N]. A/B = half, C = float, no bias.
 * tiling is computed by the host-side matmul_tiling API and passed in via GM; the kernel copies it to local and then REGISTs.
 */
#include "kernel_operator.h"
#include "lib/matmul_intf.h"

using namespace AscendC;
using namespace matmul;

typedef MatmulType<TPosition::GM, CubeFormat::ND, half>  AType;
typedef MatmulType<TPosition::GM, CubeFormat::ND, half>  BType;
typedef MatmulType<TPosition::GM, CubeFormat::ND, float> CType;
typedef MatmulType<TPosition::GM, CubeFormat::ND, float> BiasType;

extern "C" __global__ __aicore__ void k_custom(GM_ADDR a, GM_ADDR b, GM_ADDR c,
                                               GM_ADDR workspace, GM_ADDR tilingGm)
{
    // Copy the TCubeTiling on GM into local
    TCubeTiling tiling;
    auto src = reinterpret_cast<__gm__ uint32_t *>(tilingGm);
    auto dst = reinterpret_cast<uint32_t *>(&tiling);
    for (uint32_t i = 0; i < sizeof(TCubeTiling) / sizeof(uint32_t); i++) {
        dst[i] = src[i];
    }

    TPipe pipe;
    Matmul<AType, BType, CType, BiasType> mm;
    REGIST_MATMUL_OBJ(&pipe, workspace, mm, &tiling);

    GlobalTensor<half> aG, bG;
    GlobalTensor<float> cG;
    aG.SetGlobalBuffer(reinterpret_cast<__gm__ half *>(a));
    bG.SetGlobalBuffer(reinterpret_cast<__gm__ half *>(b));
    cG.SetGlobalBuffer(reinterpret_cast<__gm__ float *>(c));

    mm.SetTensorA(aG);
    mm.SetTensorB(bG);
    mm.IterateAll(cG);
    mm.End();
}
