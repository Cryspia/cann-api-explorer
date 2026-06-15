/*
 * Hand-written vector unit: Truncate (3510-only) -- same-dtype rounding to an integral value.
 * Truncate<T, roundMode>(dst, src, count): dst[i] = round(src[i]) using roundMode, with dst and src
 * the SAME floating type (half / float / bfloat16). This is NOT a dtype-changing cast (unlike Cast)
 * and NOT the math Trunc helper; it is the hardware rounding conversion kept in float form.
 * Here roundMode = CAST_TRUNC (round toward zero): 3.9 -> 3.0, -3.9 -> -3.0.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelTruncate {
public:
    __aicore__ inline KernelTruncate() {}
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
        // Round toward zero, output stays float.
        AscendC::Truncate<DT, AscendC::RoundMode::CAST_TRUNC>(zL, xL, N);
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
    KernelTruncate op;
    op.Init(x, z);
    op.Process();
}
