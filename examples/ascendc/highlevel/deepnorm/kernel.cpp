/*
 * Hand-written high-level unit: DeepNorm (adv_api/normalization, with DeepNormTiling, 3510 only needs the shape fields).
 * Formula: y = LayerNorm(alpha*x + gx) * gamma + beta, with mean/var computed over the H dimension.
 * The 3510 impl only reads tiling.{bLength,sLength,hLength,originalHLength,oneTmpSize}; the mean coefficient uses hLength internally.
 * Design [B=1,S=1,H=8], x=1, gx=0, alpha=2 -> eff=2 all equal -> var=0 -> y = beta.
 * gamma=1, beta=3 -> dst all 3, mean=2.
 */
#include "kernel_operator.h"
#include "lib/normalization/deepnorm.h"

constexpr uint32_t B = 1, S = 1, H = 8;   // H*4=32B aligned
constexpr uint32_t BS = B * S;
using DT = float;

class KernelDeepNorm {
public:
    __aicore__ inline KernelDeepNorm() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR gx, GM_ADDR beta, GM_ADDR gamma,
                                GM_ADDR z, GM_ADDR mean, GM_ADDR rstd)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, H);
        gxGm.SetGlobalBuffer((__gm__ DT *)gx, H);
        betaGm.SetGlobalBuffer((__gm__ DT *)beta, H);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, H);
        zGm.SetGlobalBuffer((__gm__ DT *)z, H);
        meanGm.SetGlobalBuffer((__gm__ DT *)mean, H);
        rstdGm.SetGlobalBuffer((__gm__ DT *)rstd, H);
        pipe.InitBuffer(inX, 1, H * sizeof(DT));
        pipe.InitBuffer(inGx, 1, H * sizeof(DT));
        pipe.InitBuffer(inBeta, 1, H * sizeof(DT));
        pipe.InitBuffer(inGamma, 1, H * sizeof(DT));
        pipe.InitBuffer(outZ, 1, H * sizeof(DT));
        pipe.InitBuffer(outMean, 1, H * sizeof(DT));
        pipe.InitBuffer(outRstd, 1, H * sizeof(DT));
        pipe.InitBuffer(tmpBuf, 4096);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gxL = inGx.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inBeta.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, H);
        AscendC::DataCopy(gxL, gxGm, H);
        AscendC::DataCopy(bL, betaGm, H);
        AscendC::DataCopy(gL, gammaGm, H);
        inX.EnQue(xL); inGx.EnQue(gxL); inBeta.EnQue(bL); inGamma.EnQue(gL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> gxL = inGx.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inBeta.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<DT> meanL = outMean.AllocTensor<DT>();
        AscendC::LocalTensor<DT> rstdL = outRstd.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::tiling::DeepNormTiling t;
        t.bLength = B;
        t.sLength = S;
        t.hLength = H;
        t.originalHLength = H;
        t.oneTmpSize = 256;   // only needs to be > 0

        const DT alpha = 2.0f;
        const DT eps = 1e-5f;
        AscendC::DeepNorm<DT, false, false>(zL, meanL, rstdL, xL, gxL, bL, gL, tmp, alpha, eps, t);

        outZ.EnQue<DT>(zL); outMean.EnQue<DT>(meanL); outRstd.EnQue<DT>(rstdL);
        inX.FreeTensor(xL); inGx.FreeTensor(gxL); inBeta.FreeTensor(bL); inGamma.FreeTensor(gL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::LocalTensor<DT> meanL = outMean.DeQue<DT>();
        AscendC::LocalTensor<DT> rstdL = outRstd.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, H);
        AscendC::DataCopy(meanGm, meanL, H);
        AscendC::DataCopy(rstdGm, rstdL, H);
        outZ.FreeTensor(zL); outMean.FreeTensor(meanL); outRstd.FreeTensor(rstdL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inGx, inBeta, inGamma;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ, outMean, outRstd;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, gxGm, betaGm, gammaGm, zGm, meanGm, rstdGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR gx, GM_ADDR beta, GM_ADDR gamma,
                                               GM_ADDR z, GM_ADDR mean, GM_ADDR rstd)
{
    KernelDeepNorm op;
    op.Init(x, gx, beta, gamma, z, mean, rstd);
    op.Process();
}
