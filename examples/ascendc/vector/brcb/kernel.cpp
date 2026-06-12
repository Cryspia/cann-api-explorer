/*
 * Hand-written vector unit: Brcb (block broadcast).
 * Brcb(dst, src0, repeatTime, {dstBlkStride, dstRepStride}):
 * one repeat takes 8 b32 elements from src, broadcasts each into a 32B block (8 float), writing 8 blocks contiguously.
 * src[8]=[0..7], repeatTime=1, dstBlkStride=1, dstRepStride=8 -> dst[b*8+k]=src[b] (64 elements total).
 */
#include "kernel_operator.h"

constexpr uint32_t SRC_N = 8;     // one repeat takes 8 elements
constexpr uint32_t DST_N = 64;    // 8 blocks x 8 elements
using DT = float;

class KernelBrcb {
public:
    __aicore__ inline KernelBrcb() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, SRC_N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, DST_N);
        pipe.InitBuffer(inX, 1, SRC_N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, DST_N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, SRC_N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::BrcbRepeatParams params(1 /*dstBlkStride*/, 8 /*dstRepStride*/);
        AscendC::Brcb<DT>(zL, xL, 1 /*repeatTime*/, params);
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, DST_N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelBrcb op;
    op.Init(x, z);
    op.Process();
}
