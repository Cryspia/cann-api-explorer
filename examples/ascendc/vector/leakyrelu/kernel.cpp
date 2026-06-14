/*
 * Hand-written vector unit: LeakyRelu (per-element leaky ReLU with a scalar negative slope, count mode).
 * LeakyRelu(dst, src, scalarValue, count): dst[i] = src[i] >= 0 ? src[i] : scalarValue * src[i].
 * Supported dtypes: half, float. Here float, negSlope=0.1.
 * src[i] = -2 (even idx) -> -0.2 ; src[i] = 3 (odd idx) -> 3.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;
constexpr DT NEG_SLOPE = 0.1f;

class KernelLeakyRelu {
public:
    __aicore__ inline KernelLeakyRelu() {}
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
        AscendC::LeakyRelu(zL, xL, NEG_SLOPE, (int32_t)N);
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
    KernelLeakyRelu op;
    op.Init(x, z);
    op.Process();
}
