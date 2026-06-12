/*
 * Hand-written vector unit: Gather (gather from src by offset, count mode).
 * Gather(dst, src, srcOffset, srcBaseAddr, count): dst[i] = src[(srcBaseAddr + srcOffset[i]) / sizeof(T)].
 * srcOffset is a byte offset. In this example srcOffset[i]=(N-1-i)*sizeof(float) -> dst[i]=src[N-1-i] (reverse-order gather).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelGather {
public:
    __aicore__ inline KernelGather() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR off, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        offGm.SetGlobalBuffer((__gm__ uint32_t *)off, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inOff, 1, N * sizeof(uint32_t));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<uint32_t> oL = inOff.AllocTensor<uint32_t>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(oL, offGm, N);
        inX.EnQue(xL); inOff.EnQue(oL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<uint32_t> oL = inOff.DeQue<uint32_t>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Gather<DT>(zL, xL, oL, (uint32_t)0, N);
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL); inOff.FreeTensor(oL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inOff;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
    AscendC::GlobalTensor<uint32_t> offGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR off, GM_ADDR z)
{
    KernelGather op;
    op.Init(x, off, z);
    op.Process();
}
