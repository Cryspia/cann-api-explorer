/*
 * Hand-written vector unit: DeInterleave (3510-only) -- inverse of Interleave.
 * DeInterleave(dst0, dst1, src, srcCount): src is an interleaved stream [a0,b0,a1,b1,...]
 * of length srcCount (must be even); it is split back into dst0 = [a0,a1,a2,...] and
 * dst1 = [b0,b1,b2,...], each of length srcCount/2.
 * Here src[i] = i, so even positions (a) -> dst0 = [0,2,4,...], odd positions (b) -> dst1 = [1,3,5,...].
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;          // interleaved source length (even); each output is N/2
using DT = float;

class KernelDeInterleave {
public:
    __aicore__ inline KernelDeInterleave() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z0, GM_ADDR z1)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        z0Gm.SetGlobalBuffer((__gm__ DT *)z0, N / 2);
        z1Gm.SetGlobalBuffer((__gm__ DT *)z1, N / 2);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ0, 1, (N / 2) * sizeof(DT));
        pipe.InitBuffer(outZ1, 1, (N / 2) * sizeof(DT));
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
        AscendC::LocalTensor<DT> d0 = outZ0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> d1 = outZ1.AllocTensor<DT>();
        // Single-source DeInterleave: split [a0,b0,a1,b1,...] into dst0(a) and dst1(b).
        AscendC::DeInterleave<DT>(d0, d1, xL, (int32_t)N);
        outZ0.EnQue<DT>(d0);
        outZ1.EnQue<DT>(d1);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d0 = outZ0.DeQue<DT>();
        AscendC::LocalTensor<DT> d1 = outZ1.DeQue<DT>();
        AscendC::DataCopy(z0Gm, d0, N / 2);
        AscendC::DataCopy(z1Gm, d1, N / 2);
        outZ0.FreeTensor(d0);
        outZ1.FreeTensor(d1);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ0, outZ1;
    AscendC::GlobalTensor<DT> xGm, z0Gm, z1Gm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z0, GM_ADDR z1)
{
    KernelDeInterleave op;
    op.Init(x, z0, z1);
    op.Process();
}
