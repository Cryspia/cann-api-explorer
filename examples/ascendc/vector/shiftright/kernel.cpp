/*
 * Hand-written vector unit: ShiftRight (per-element logical right shift, two input tensors, count mode).
 * ShiftRight<T,U>(dst, src0, src1, count): dst[i] = src0[i] >> src1[i].
 * Supported dtype combinations on this SoC include T=int32, U=int32 (also int64/uint64, uint32).
 * src0[i]=16, src1[i]=2 -> dst[i]=4.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using T = int32_t;   // src0 / dst
using U = int32_t;   // src1 (shift amount)

class KernelShiftRight {
public:
    __aicore__ inline KernelShiftRight() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
    {
        x0Gm.SetGlobalBuffer((__gm__ T *)x0, N);
        x1Gm.SetGlobalBuffer((__gm__ U *)x1, N);
        zGm.SetGlobalBuffer((__gm__ T *)z, N);
        pipe.InitBuffer(inX0, 1, N * sizeof(T));
        pipe.InitBuffer(inX1, 1, N * sizeof(U));
        pipe.InitBuffer(outZ, 1, N * sizeof(T));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<T> a = inX0.AllocTensor<T>();
        AscendC::LocalTensor<U> b = inX1.AllocTensor<U>();
        AscendC::DataCopy(a, x0Gm, N);
        AscendC::DataCopy(b, x1Gm, N);
        inX0.EnQue(a); inX1.EnQue(b);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<T> a = inX0.DeQue<T>();
        AscendC::LocalTensor<U> b = inX1.DeQue<U>();
        AscendC::LocalTensor<T> zL = outZ.AllocTensor<T>();
        AscendC::ShiftRight<T, U>(zL, a, b, N);
        outZ.EnQue<T>(zL);
        inX0.FreeTensor(a); inX1.FreeTensor(b);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<T> zL = outZ.DeQue<T>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX0, inX1;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<T> x0Gm, zGm;
    AscendC::GlobalTensor<U> x1Gm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
{
    KernelShiftRight op;
    op.Init(x0, x1, z);
    op.Process();
}
