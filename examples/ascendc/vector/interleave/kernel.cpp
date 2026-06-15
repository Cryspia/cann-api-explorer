/*
 * Hand-written vector unit: Interleave (3510-only) -- interleave two source vectors.
 * Interleave(dst0, dst1, src0, src1, count): given `count` elements, the lower half
 * of src0/src1 is interleaved element-by-element into dst0 ([a0,b0,a1,b1,...]) and the
 * upper half into dst1. Concatenating dst0 then dst1 reproduces the fully interleaved
 * stream [a0,b0,a1,b1,...] over all `count` pairs. count must be even.
 * Here src0 is all 1.0 and src1 is all 2.0, so the concatenated output is [1,2,1,2,...].
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;          // number of elements per source vector
using DT = float;

class KernelInterleave {
public:
    __aicore__ inline KernelInterleave() {}
    __aicore__ inline void Init(GM_ADDR z0, GM_ADDR z1)
    {
        z0Gm.SetGlobalBuffer((__gm__ DT *)z0, N);
        z1Gm.SetGlobalBuffer((__gm__ DT *)z1, N);
        pipe.InitBuffer(outZ0, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ1, 1, N * sizeof(DT));
        pipe.InitBuffer(src0Buf, N * sizeof(DT));
        pipe.InitBuffer(src1Buf, N * sizeof(DT));
    }
    __aicore__ inline void Process() { Compute(); CopyOut(); }
private:
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s0 = src0Buf.Get<DT>();
        AscendC::LocalTensor<DT> s1 = src1Buf.Get<DT>();
        AscendC::LocalTensor<DT> d0 = outZ0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> d1 = outZ1.AllocTensor<DT>();

        // src0 = all 1.0, src1 = all 2.0
        AscendC::Duplicate<DT>(s0, (DT)1.0f, N);
        AscendC::Duplicate<DT>(s1, (DT)2.0f, N);
        AscendC::PipeBarrier<PIPE_V>();
        // Interleave N elements: dst0 = first N/2 interleaved pairs, dst1 = remaining pairs.
        AscendC::Interleave<DT>(d0, d1, s0, s1, (int32_t)N);

        outZ0.EnQue<DT>(d0);
        outZ1.EnQue<DT>(d1);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d0 = outZ0.DeQue<DT>();
        AscendC::LocalTensor<DT> d1 = outZ1.DeQue<DT>();
        AscendC::DataCopy(z0Gm, d0, N);
        AscendC::DataCopy(z1Gm, d1, N);
        outZ0.FreeTensor(d0);
        outZ1.FreeTensor(d1);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ0, outZ1;
    AscendC::TBuf<AscendC::TPosition::VECCALC> src0Buf, src1Buf;
    AscendC::GlobalTensor<DT> z0Gm, z1Gm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR z0, GM_ADDR z1)
{
    KernelInterleave op;
    op.Init(z0, z1);
    op.Process();
}
