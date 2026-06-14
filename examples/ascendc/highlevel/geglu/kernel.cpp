/*
 * Hand-written AscendC high-level math unit: GeGLU (adv_api, simple count mode, no Tiling needed).
 * GeGLU<T>(dst, src0, src1, count): gated GELU, per the header dst = src0 * GeLU(src1).
 * src0=1.0, src1=0.0 -> GeLU(0)=0 -> dst = 1 * 0 = 0.0.
 */
#include "kernel_operator.h"
#include "lib/math/geglu.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelGeGLU {
public:
    __aicore__ inline KernelGeGLU() {}
    __aicore__ inline void Init(GM_ADDR a, GM_ADDR b, GM_ADDR z)
    {
        aGm.SetGlobalBuffer((__gm__ DT *)a, N);
        bGm.SetGlobalBuffer((__gm__ DT *)b, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inA, 1, N * sizeof(DT));
        pipe.InitBuffer(inB, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> aL = inA.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inB.AllocTensor<DT>();
        AscendC::DataCopy(aL, aGm, N);
        AscendC::DataCopy(bL, bGm, N);
        inA.EnQue(aL); inB.EnQue(bL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> aL = inA.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inB.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::GeGLU<DT>(zL, aL, bL, N);                 // dst = src0 * GeLU(src1)
        outZ.EnQue<DT>(zL);
        inA.FreeTensor(aL); inB.FreeTensor(bL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inA, inB;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> aGm, bGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR a, GM_ADDR b, GM_ADDR z)
{
    KernelGeGLU op;
    op.Init(a, b, z);
    op.Process();
}
