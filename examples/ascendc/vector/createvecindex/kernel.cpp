/*
 * Hand-written vector unit: CreateVecIndex (generates an arithmetic-progression index vector, count mode).
 * CreateVecIndex(dst, firstValue, count): dst[i] = firstValue + i.
 * N=64, firstValue=0 -> dst[i]=i.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelCreateVecIndex {
public:
    __aicore__ inline KernelCreateVecIndex() {}
    __aicore__ inline void Init(GM_ADDR z)
    {
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { Compute(); CopyOut(); }
private:
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::CreateVecIndex<DT>(zL, (DT)0.0f, N);
        outZ.EnQue<DT>(zL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR z)
{
    KernelCreateVecIndex op;
    op.Init(z);
    op.Process();
}
