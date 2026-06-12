/*
 * Hand-written vector unit: Cast float->half (complements the existing float->int32 cast unit, verifies the down-precision conversion path).
 * Cast<DST,SRC,RoundMode>(dst, src, count): DST=half, SRC=float.
 * x[i]=i (0..255, exactly representable in half) -> z[i]=(half)i.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using ST = float;
using DT = half;

class KernelCastF2H {
public:
    __aicore__ inline KernelCastF2H() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ ST *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(ST));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> xL = inX.AllocTensor<ST>();
        AscendC::DataCopy(xL, xGm, N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> xL = inX.DeQue<ST>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Cast(zL, xL, AscendC::RoundMode::CAST_NONE, N);
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
    AscendC::GlobalTensor<ST> xGm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelCastF2H op;
    op.Init(x, z);
    op.Process();
}
