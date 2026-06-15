/*
 * Hand-written vector unit: AddReluCast (fused add + ReLU + type conversion, count mode).
 * AddReluCast<T,U>(dst, src0, src1, count): dst = cast_to_T( relu(src0 + src1) ).
 * Supported dtype combination on this SoC (Level 2): src(U)=float, dst(T)=half.
 * No deqScale needed (unlike AddDeqRelu).
 * src0[i] alternates -3/2, src1=1 -> relu(-2)=0 and relu(3)=3, cast to half.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 256;
using ST = float;          // src type (U)
using DT = half;           // dst type (T)

class KernelAddReluCast {
public:
    __aicore__ inline KernelAddReluCast() {}
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
        AscendC::AddReluCast<DT, ST>(zL, x0L, x1L, N);          // dst = cast(relu(src0 + src1))
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
    KernelAddReluCast op;
    op.Init(x0, x1, z);
    op.Process();
}
