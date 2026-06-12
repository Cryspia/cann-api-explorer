/*
 * Hand-written vector unit: Axpy (y = a*x + y, count mode).
 * Axpy<T,U>(dst, src, scalarValue, count): dst[i] += scalarValue * src[i] (accumulates into dst).
 * Preset dst to all 1.0, src[i]=i, scalar=2.0 -> dst[i] = 1 + 2*i.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelAxpy {
public:
    __aicore__ inline KernelAxpy() {}
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
        AscendC::Duplicate<DT>(zL, (DT)1.0f, N);                 // preset dst=1
        AscendC::PipeBarrier<PIPE_V>();
        AscendC::Axpy<DT, DT>(zL, xL, (DT)2.0f, N);              // dst += 2*x
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
    KernelAxpy op;
    op.Init(x, z);
    op.Process();
}
