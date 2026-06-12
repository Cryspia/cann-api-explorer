/*
 * Hand-written unit: SetAtomicAdd (atomic add to GM, multi-core scenario).
 * SetAtomicAdd<T>() sets subsequent DataCopy(UB->GM) to atomic-add mode; SetAtomicNone() restores it.
 * launch 8 cores: each core atomic-adds all 1.0 into the same GM region -> each element = 8.0 (host must zero GM first).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelAtomicAdd {
public:
    __aicore__ inline KernelAtomicAdd() {}
    __aicore__ inline void Init(GM_ADDR z)
    {
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(outQ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process()
    {
        AscendC::LocalTensor<DT> ones = outQ.AllocTensor<DT>();
        AscendC::Duplicate<DT>(ones, (DT)1.0f, N);
        outQ.EnQue(ones);
        AscendC::LocalTensor<DT> o = outQ.DeQue<DT>();
        AscendC::SetAtomicAdd<DT>();
        AscendC::DataCopy(zGm, o, N);     // atomic add to GM
        AscendC::SetAtomicNone();
        outQ.FreeTensor(o);
    }
private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQ;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR z)
{
    KernelAtomicAdd op;
    op.Init(z);
    op.Process();
}
