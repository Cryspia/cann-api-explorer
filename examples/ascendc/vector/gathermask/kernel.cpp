/*
 * Hand-written vector unit: GatherMask (stream-compaction element selection by pattern).
 * GatherMask<T,mode>(dst, src0, src1Pattern(uint8), reduceMode, mask, GatherMaskParams, rsvdCnt):
 * select src0 elements by a fixed pattern and pack them contiguously into dst; rsvdCnt returns the number selected.
 * pattern=1 (measured): selects even-index elements -> dst[i]=src[2i], rsvdCnt=N/2.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelGatherMask {
public:
    __aicore__ inline KernelGatherMask() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z, GM_ADDR rsvd)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        rsvdGm.SetGlobalBuffer((__gm__ uint32_t *)rsvd, 8);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
        pipe.InitBuffer(rsvdBuf, 32);
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
        uint64_t rsvdCnt = 0;
        AscendC::GatherMaskParams params(1 /*src0BlockStride*/, 1 /*repeatTimes*/, 8 /*src0RepStride*/, 8 /*src1RepStride*/);
        AscendC::GatherMask<DT>(zL, xL, (uint8_t)1, false /*reduceMode*/, (uint32_t)N, params, rsvdCnt);
        AscendC::LocalTensor<uint32_t> r = rsvdBuf.Get<uint32_t>();
        r.SetValue(0, (uint32_t)rsvdCnt);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::DataCopy(rsvdGm, r, 8);
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
    AscendC::TBuf<AscendC::TPosition::VECCALC> rsvdBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
    AscendC::GlobalTensor<uint32_t> rsvdGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z, GM_ADDR rsvd)
{
    KernelGatherMask op;
    op.Init(x, z, rsvd);
    op.Process();
}
