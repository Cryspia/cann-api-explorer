/*
 * Hand-written vector unit: Compare (per-element comparison of two vectors into a mask, count mode) + Select for verification.
 * Compare<T,U>(maskDst, src0, src1, CMPMODE, count): each bit of maskDst = (src0[i] cmp src1[i]).
 * Use Select to turn the mask into float for easy verification: src0[i]=i, src1=31.5, LT -> mask=(i<31.5) -> z[i]=(i<32)?1:0.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelCompare {
public:
    __aicore__ inline KernelCompare() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
        pipe.InitBuffer(s1Buf, N * sizeof(DT));
        pipe.InitBuffer(oneBuf, N * sizeof(DT));
        pipe.InitBuffer(zeroBuf, N * sizeof(DT));
        pipe.InitBuffer(maskBuf, N * sizeof(uint8_t));
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
        AscendC::LocalTensor<DT> s1 = s1Buf.Get<DT>();
        AscendC::LocalTensor<DT> ones = oneBuf.Get<DT>();
        AscendC::LocalTensor<DT> zeros = zeroBuf.Get<DT>();
        AscendC::LocalTensor<uint8_t> mask = maskBuf.Get<uint8_t>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();

        AscendC::Duplicate<DT>(s1, (DT)31.5f, N);
        AscendC::Duplicate<DT>(ones, (DT)1.0f, N);
        AscendC::Duplicate<DT>(zeros, (DT)0.0f, N);
        AscendC::PipeBarrier<PIPE_V>();
        AscendC::Compare(mask, xL, s1, AscendC::CMPMODE::LT, N);
        AscendC::PipeBarrier<PIPE_V>();
        AscendC::Select(zL, mask, ones, zeros, AscendC::SELMODE::VSEL_TENSOR_TENSOR_MODE, N);

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
    AscendC::TBuf<AscendC::TPosition::VECCALC> s1Buf, oneBuf, zeroBuf, maskBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelCompare op;
    op.Init(x, z);
    op.Process();
}
