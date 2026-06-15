/*
 * Hand-written vector unit: AddDeqRelu (fused add + dequant + ReLU, count mode).
 * AddDeqRelu(dst<half>, src0<int32>, src1<int32>, count): dst = relu( (src0 + src1) * deqScale ).
 * The dequant scale is the scalar g_deqValue set via SetDeqScale(half). The 3510 impl computes
 *   max( float(src0+src1) * (1/131072) * g_deqValue * 131072, 0 )
 * where the 1/131072 and 131072 factors cancel, leaving max((src0+src1)*deqScale, 0).
 *
 * deqScale = 2.0. src0 alternates 1/-5, src1 alternates 2/1:
 *   even: (1+2)*2 = 6 ; odd: (-5+1)*2 = -8 -> relu -> 0.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using ST = int32_t;        // src type
using DT = half;           // dst type

class KernelAddDeqRelu {
public:
    __aicore__ inline KernelAddDeqRelu() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
    {
        x0Gm.SetGlobalBuffer((__gm__ ST *)x0, N);
        x1Gm.SetGlobalBuffer((__gm__ ST *)x1, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX0, 1, N * sizeof(ST));
        pipe.InitBuffer(inX1, 1, N * sizeof(ST));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> x0L = inX0.AllocTensor<ST>();
        AscendC::LocalTensor<ST> x1L = inX1.AllocTensor<ST>();
        AscendC::DataCopy(x0L, x0Gm, N);
        AscendC::DataCopy(x1L, x1Gm, N);
        inX0.EnQue(x0L);
        inX1.EnQue(x1L);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> x0L = inX0.DeQue<ST>();
        AscendC::LocalTensor<ST> x1L = inX1.DeQue<ST>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();

        AscendC::SetDeqScale((half)2.0);                        // sets scalar g_deqValue = 2.0
        AscendC::AddDeqRelu(zL, x0L, x1L, (int32_t)N);          // dst = relu((src0+src1)*deqScale)

        outZ.EnQue<DT>(zL);
        inX0.FreeTensor(x0L);
        inX1.FreeTensor(x1L);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX0, inX1;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<ST> x0Gm, x1Gm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
{
    KernelAddDeqRelu op;
    op.Init(x0, x1, z);
    op.Process();
}
