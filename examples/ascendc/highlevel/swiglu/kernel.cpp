/*
 * Hand-written high-level unit: SwiGLU (adv_api activation, count mode).
 * SwiGLU(dst, src0, src1, beta, count): dst = src0 * silu_beta(src1),
 *   where silu_beta(x) = x / (1 + exp(-beta * x)).
 * With beta = 0 the gate degenerates to silu_0(x) = x / 2, so dst = src0 * src1 / 2 (exact in fp32).
 * Inputs: src0 = 4.0, src1 = 3.0, beta = 0 -> dst = 4 * 3 / 2 = 6.0.
 */
#include "kernel_operator.h"
#include "lib/activation/swiglu.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelSwiGLU {
public:
    __aicore__ inline KernelSwiGLU() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
    {
        x0Gm.SetGlobalBuffer((__gm__ DT *)x0, N);
        x1Gm.SetGlobalBuffer((__gm__ DT *)x1, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX0, 1, N * sizeof(DT));
        pipe.InitBuffer(inX1, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> x0L = inX0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.AllocTensor<DT>();
        AscendC::DataCopy(x0L, x0Gm, N);
        AscendC::DataCopy(x1L, x1Gm, N);
        inX0.EnQue(x0L);
        inX1.EnQue(x1L);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> x0L = inX0.DeQue<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();

        // beta = 0 -> silu_0(x1) = x1 / 2 -> dst = x0 * x1 / 2
        AscendC::SwiGLU<DT>(zL, x0L, x1L, 0.0f, N);

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
    AscendC::GlobalTensor<DT> x0Gm, x1Gm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
{
    KernelSwiGLU op;
    op.Init(x0, x1, z);
    op.Process();
}
