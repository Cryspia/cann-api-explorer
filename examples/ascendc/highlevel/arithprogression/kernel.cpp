/*
 * Hand-written high-level unit: ArithProgression (adv_api index, exposed as AscendC::Arange).
 * Arange<T>(dst, firstValue, diffValue, count): dst[i] = firstValue + i * diffValue.
 * firstValue=0, diffValue=1 -> dst[i] = i (0,1,2,...,63). No input tensor / no tmp needed.
 */
#include "kernel_operator.h"
#include "lib/index/arithprogression.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelArange {
public:
    __aicore__ inline KernelArange() {}
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
        AscendC::Arange<DT>(zL, (DT)0.0f, (DT)1.0f, N);   // dst[i] = i
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
    KernelArange op;
    op.Init(z);
    op.Process();
}
