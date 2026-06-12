/*
 * Hand-written vector unit: MulCast (per-element multiply + type conversion fused, count mode).
 * MulCast<T,U>(dst, src0, src1, count): dst[i] = (T)(src0[i] * src1[i]).
 * Supported dtype combinations: T=int32,U=int64 / T=int8|uint8,U=half. In this example src=int64 -> dst=int32.
 * src0[i]=i, src1[i]=2 (int64) -> dst[i]=2i (int32).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using ST = int64_t;   // src
using DT = int32_t;   // dst

class KernelMulCast {
public:
    __aicore__ inline KernelMulCast() {}
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
        AscendC::LocalTensor<ST> a = inX0.AllocTensor<ST>();
        AscendC::LocalTensor<ST> b = inX1.AllocTensor<ST>();
        AscendC::DataCopy(a, x0Gm, N);
        AscendC::DataCopy(b, x1Gm, N);
        inX0.EnQue(a); inX1.EnQue(b);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> a = inX0.DeQue<ST>();
        AscendC::LocalTensor<ST> b = inX1.DeQue<ST>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::MulCast<DT, ST>(zL, a, b, N);
        outZ.EnQue<DT>(zL);
        inX0.FreeTensor(a); inX1.FreeTensor(b);
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
    KernelMulCast op;
    op.Init(x0, x1, z);
    op.Process();
}
