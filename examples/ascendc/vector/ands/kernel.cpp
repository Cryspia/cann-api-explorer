/*
 * Hand-written vector unit: Ands (per-element bitwise AND of a tensor with a scalar, count mode).
 * Ands(dst, src0, src1, count): dst[i] = src0[i] & src1, where src1 is a scalar.
 * Level-2 supported integer dtypes on this SoC: int16/uint16/int64/uint64 (int32 is NOT supported,
 * so this unit uses int16_t). src0[i]=6, scalar=3 -> dst[i]=2.
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using DT = int16_t;
constexpr DT SCALAR = 3;

class KernelAnds {
public:
    __aicore__ inline KernelAnds() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Ands(zL, xL, SCALAR, (int32_t)N);
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
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelAnds op;
    op.Init(x, z);
    op.Process();
}
