/*
 * Hand-written scalar sample: ScalarCast (scalar cast, with rounding mode).
 * ScalarCast<T,U,RoundMode>(valueIn): casts scalar T to U (CAST_RINT rounds to nearest).
 * 3.7f --CAST_RINT--> 4 (int32). Uses Duplicate to fill GM for easy host verification.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 8;
using DT = int32_t;

class KernelScalarCast {
public:
    __aicore__ inline KernelScalarCast() {}
    __aicore__ inline void Init(GM_ADDR z)
    {
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(outQ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process()
    {
        DT v = AscendC::ScalarCast<float, DT, AscendC::RoundMode::CAST_RINT>(3.7f);
        AscendC::LocalTensor<DT> zL = outQ.AllocTensor<DT>();
        AscendC::Duplicate<DT>(zL, v, N);
        outQ.EnQue(zL);
        AscendC::LocalTensor<DT> o = outQ.DeQue<DT>();
        AscendC::DataCopy(zGm, o, N);
        outQ.FreeTensor(o);
    }
private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQ;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR z)
{
    KernelScalarCast op;
    op.Init(z);
    op.Process();
}
