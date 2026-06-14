/*
 * Hand-written vector unit: MulsCast (fused multiply-by-scalar + type conversion, count mode).
 * MulsCast(dst, src0, src1, count): dst[i] = (DstType)(src0[i] * src1), where src1 is a scalar.
 * Supported dtype combination on this SoC: src=float, scalar=float, dst=half.
 * src0[i]=i, scalar=0.5 -> dst[i] = half(0.5 * i).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;
using ST = float;          // src
using DT = half;           // dst
constexpr ST SCALAR = 0.5f;

class KernelMulsCast {
public:
    __aicore__ inline KernelMulsCast() {}
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
        AscendC::MulsCast(zL, xL, SCALAR, (uint32_t)N);
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
    KernelMulsCast op;
    op.Init(x, z);
    op.Process();
}
