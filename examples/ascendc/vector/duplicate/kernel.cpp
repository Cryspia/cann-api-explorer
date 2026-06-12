/*
 * Hand-written vector unit: Duplicate (fill dst entirely with a scalar, count mode).
 * Duplicate(dst, scalarValue, count): dst[i] = scalarValue, i in [0,count).
 * N=64, scalar=3.0 -> dst all 3.0.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelDuplicate {
public:
    __aicore__ inline KernelDuplicate() {}
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
        AscendC::Duplicate<DT>(zL, (DT)3.0f, N);
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
    KernelDuplicate op;
    op.Init(z);
    op.Process();
}
