/*
 * Hand-written AscendC high-level math unit: Fma (adv_api, simple count mode, no Tiling needed).
 * Fma<config,T>(dst, src0, src1, src2, count): fused multiply-add, dst[i] = src0[i] * src1[i] + src2[i].
 * src0=2.0, src1=3.0, src2=1.0 -> dst = 2*3 + 1 = 7.0.
 */
#include "kernel_operator.h"
#include "lib/math/fma.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelFma {
public:
    __aicore__ inline KernelFma() {}
    __aicore__ inline void Init(GM_ADDR a, GM_ADDR b, GM_ADDR c, GM_ADDR z)
    {
        aGm.SetGlobalBuffer((__gm__ DT *)a, N);
        bGm.SetGlobalBuffer((__gm__ DT *)b, N);
        cGm.SetGlobalBuffer((__gm__ DT *)c, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inA, 1, N * sizeof(DT));
        pipe.InitBuffer(inB, 1, N * sizeof(DT));
        pipe.InitBuffer(inC, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> aL = inA.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inB.AllocTensor<DT>();
        AscendC::LocalTensor<DT> cL = inC.AllocTensor<DT>();
        AscendC::DataCopy(aL, aGm, N);
        AscendC::DataCopy(bL, bGm, N);
        AscendC::DataCopy(cL, cGm, N);
        inA.EnQue(aL); inB.EnQue(bL); inC.EnQue(cL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> aL = inA.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inB.DeQue<DT>();
        AscendC::LocalTensor<DT> cL = inC.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::Fma(zL, aL, bL, cL, N);                  // dst = src0 * src1 + src2
        outZ.EnQue<DT>(zL);
        inA.FreeTensor(aL); inB.FreeTensor(bL); inC.FreeTensor(cL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inA, inB, inC;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> aGm, bGm, cGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR a, GM_ADDR b, GM_ADDR c, GM_ADDR z)
{
    KernelFma op;
    op.Init(a, b, c, z);
    op.Process();
}
