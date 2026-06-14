/*
 * Hand-written vector unit: Mull (wide integer multiply, count mode).
 * Mull<T>(dst0, dst1, src0, src1, count): dst0 = low 32 bits, dst1 = high 32 bits of src0*src1.
 * T must be int32_t / uint32_t. Test src0=2, src1=3 -> product 6 -> dst0(low)=6, dst1(high)=0.
 * Only the low-half output dst0 is checked on the host.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using DT = int32_t;

class KernelMull {
public:
    __aicore__ inline KernelMull() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR zLow, GM_ADDR zHigh)
    {
        x0Gm.SetGlobalBuffer((__gm__ DT *)x0, N);
        x1Gm.SetGlobalBuffer((__gm__ DT *)x1, N);
        zLowGm.SetGlobalBuffer((__gm__ DT *)zLow, N);
        zHighGm.SetGlobalBuffer((__gm__ DT *)zHigh, N);
        pipe.InitBuffer(inX0, 1, N * sizeof(DT));
        pipe.InitBuffer(inX1, 1, N * sizeof(DT));
        pipe.InitBuffer(outLow, 1, N * sizeof(DT));
        pipe.InitBuffer(outHigh, 1, N * sizeof(DT));
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
        AscendC::LocalTensor<DT> lowL = outLow.AllocTensor<DT>();
        AscendC::LocalTensor<DT> highL = outHigh.AllocTensor<DT>();
        AscendC::Mull<DT>(lowL, highL, x0L, x1L, N);             // dst0=low, dst1=high of src0*src1
        outLow.EnQue<DT>(lowL);
        outHigh.EnQue<DT>(highL);
        inX0.FreeTensor(x0L);
        inX1.FreeTensor(x1L);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> lowL = outLow.DeQue<DT>();
        AscendC::LocalTensor<DT> highL = outHigh.DeQue<DT>();
        AscendC::DataCopy(zLowGm, lowL, N);
        AscendC::DataCopy(zHighGm, highL, N);
        outLow.FreeTensor(lowL);
        outHigh.FreeTensor(highL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX0, inX1;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outLow, outHigh;
    AscendC::GlobalTensor<DT> x0Gm, x1Gm, zLowGm, zHighGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR zLow, GM_ADDR zHigh)
{
    KernelMull op;
    op.Init(x0, x1, zLow, zHigh);
    op.Process();
}
