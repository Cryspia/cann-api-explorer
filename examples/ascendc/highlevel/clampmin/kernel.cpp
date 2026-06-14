/*
 * Hand-written AscendC high-level math unit: ClampMin (adv_api, simple count mode, no Tiling needed).
 * ClampMin<T>(dst, src, scalar, count): dst[i] = max(src[i], scalar) (clamp values below scalar up to scalar).
 * src filled with 1.0, scalar=3.0 -> dst ~ 3.0.
 */
#include "kernel_operator.h"
#include "lib/math/clamp.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelClampMin {
public:
    __aicore__ inline KernelClampMin() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::ClampMin<DT>(zL, xL, (DT)3.0f, N);            // dst = max(src, 3)
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelClampMin op;
    op.Init(x, z);
    op.Process();
}
