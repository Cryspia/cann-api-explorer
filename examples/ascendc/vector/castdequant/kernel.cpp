/*
 * Hand-written vector unit: CastDequant (cast + dequant, count mode).
 * CastDequant<T,U>(dst, src, count): dst = cast_and_dequant(src) using scalar deqScale.
 * (CastDeq is the deprecated alias of CastDequant per the header comment.)
 * For the int32 -> half path the 3510 impl computes
 *   float(src) * (1/131072) * g_deqValue * 131072
 * where the 131072 factors cancel, leaving dst = src * g_deqValue (half).
 * deqScale set via SetDeqScale(half).
 *
 * deqScale = 2.0. src(int32) = 3 -> dst = 3 * 2 = 6 (half).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using ST = int32_t;        // src type (U)
using DT = half;           // dst type (T)

class KernelCastDequant {
public:
    __aicore__ inline KernelCastDequant() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ ST *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(ST));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> xL = inX.AllocTensor<ST>();
        AscendC::DataCopy(xL, xGm, N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> xL = inX.DeQue<ST>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();

        AscendC::SetDeqScale((half)2.0);                        // sets scalar g_deqValue = 2.0
        AscendC::CastDequant<DT, ST>(zL, xL, N);                // dst = src * deqScale (int32 -> half)

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
    AscendC::GlobalTensor<ST> xGm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelCastDequant op;
    op.Init(x, z);
    op.Process();
}
