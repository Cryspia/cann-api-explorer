/*
 * Hand-written AscendC high-level math unit: SinCos (adv_api, simple count mode, no Tiling needed).
 * SinCos(dstSin, dstCos, src, count): two outputs, dstSin = sin(src), dstCos = cos(src).
 * src filled with 0.0 -> dstSin ~ 0.0, dstCos ~ 1.0.
 */
#include "kernel_operator.h"
#include "lib/math/sincos.h"

constexpr uint32_t N = 64;
using DT = float;

class KernelSinCos {
public:
    __aicore__ inline KernelSinCos() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR sinOut, GM_ADDR cosOut)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        sinGm.SetGlobalBuffer((__gm__ DT *)sinOut, N);
        cosGm.SetGlobalBuffer((__gm__ DT *)cosOut, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outSin, 1, N * sizeof(DT));
        pipe.InitBuffer(outCos, 1, N * sizeof(DT));
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
        AscendC::LocalTensor<DT> sinL = outSin.AllocTensor<DT>();
        AscendC::LocalTensor<DT> cosL = outCos.AllocTensor<DT>();
        AscendC::SinCos(sinL, cosL, xL, N);            // dstSin=sin(src), dstCos=cos(src)
        outSin.EnQue<DT>(sinL);
        outCos.EnQue<DT>(cosL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> sinL = outSin.DeQue<DT>();
        AscendC::LocalTensor<DT> cosL = outCos.DeQue<DT>();
        AscendC::DataCopy(sinGm, sinL, N);
        AscendC::DataCopy(cosGm, cosL, N);
        outSin.FreeTensor(sinL);
        outCos.FreeTensor(cosL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outSin, outCos;
    AscendC::GlobalTensor<DT> xGm, sinGm, cosGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR sinOut, GM_ADDR cosOut)
{
    KernelSinCos op;
    op.Init(x, sinOut, cosOut);
    op.Process();
}
