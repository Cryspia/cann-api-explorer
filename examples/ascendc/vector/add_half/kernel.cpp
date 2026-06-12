/*
 * Hand-written vector unit: the half variant of Add (verifies the vector compute path for the half data type).
 * DT=half, single core: a=1.0, b=2.0 -> c=3.0. Same API as the float version of Add, only the dtype differs.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;   // 256*2=512B aligned
using DT = half;

class KernelAddHalf {
public:
    __aicore__ inline KernelAddHalf() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        yGm.SetGlobalBuffer((__gm__ DT *)y, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inY, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> yL = inY.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(yL, yGm, N);
        inX.EnQue(xL); inY.EnQue(yL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> yL = inY.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Add(zL, xL, yL, N);
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL); inY.FreeTensor(yL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inY;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, yGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z)
{
    KernelAddHalf op;
    op.Init(x, y, z);
    op.Process();
}
