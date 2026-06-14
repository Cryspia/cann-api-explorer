/*
 * Hand-written vector unit: FusedMulAdd (count mode).
 * FusedMulAdd<T>(dst, src0, src1, count): dst[i] = src0[i] * dst[i] + src1[i] (accumulates into dst).
 * dst must be preset first (Duplicate). Preset dst=1, src0=2, src1=3 -> 2*1+3 = 5.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using DT = float;

class KernelFusedMulAdd {
public:
    __aicore__ inline KernelFusedMulAdd() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
    {
        x0Gm.SetGlobalBuffer((__gm__ DT *)x0, N);
        x1Gm.SetGlobalBuffer((__gm__ DT *)x1, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX0, 1, N * sizeof(DT));
        pipe.InitBuffer(inX1, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> x0L = inX0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.AllocTensor<DT>();
        AscendC::DataCopy(x0L, x0Gm, N);
        AscendC::DataCopy(x1L, x1Gm, N);
        inX0.EnQue(x0L);
        inX1.EnQue(x1L);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> x0L = inX0.DeQue<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Duplicate<DT>(zL, (DT)1.0f, N);                 // preset dst=1
        AscendC::PipeBarrier<PIPE_V>();
        AscendC::FusedMulAdd<DT>(zL, x0L, x1L, N);               // dst = src0 * dst + src1
        outZ.EnQue<DT>(zL);
        inX0.FreeTensor(x0L);
        inX1.FreeTensor(x1L);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX0, inX1;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> x0Gm, x1Gm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
{
    KernelFusedMulAdd op;
    op.Init(x0, x1, z);
    op.Process();
}
