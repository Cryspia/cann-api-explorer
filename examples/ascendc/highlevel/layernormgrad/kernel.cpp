/*
 * Hand-written high-level unit: LayerNormGrad (adv_api/normalization, with LayerNormGradTiling, this arch=3510).
 * Backward of LayerNorm. Two outputs:
 *   resForGamma = (x - mean) * rstd            (rstd = 1/sqrt(var+eps)), independent of dy
 *   outputPdX   = x1*rstd + pdVar*(2/H)*x2 + pdMean*(1/H), where x1 = dy*gamma, x2 = x-mean
 * The 3510 impl only reads tiling.{stackBufferSize,bLength,sLength,hLength,lastDimValueBack,lastDimValueBackMulTwo};
 * lastDimValueBack / lastDimValueBackMulTwo are 1/H and 2/H stored as float *bit patterns* in uint32 fields.
 *
 * Verifiable design [B=1,S=1,H=8]: dy=0 -> x1=0 -> pdVar=0 and pdMean=0 -> outputPdX=0.
 * Also x = mean = 5.0 (so var=0) -> x2 = x-mean = 0 -> resForGamma=0. Both outputs are 0. Verified on host.
 */
#include "kernel_operator.h"
#include "lib/normalization/layernormgrad.h"

constexpr uint32_t B = 1, S = 1, H = 8;     // H*4=32B aligned
constexpr uint32_t BS = B * S;              // 1 mean/variance element (impl reads index j<BS)
constexpr uint32_t BS_PAD = 8;              // DataCopy granularity must be 32B-aligned -> pad mean/var to 8 floats
constexpr uint32_t TOTAL = B * S * H;       // 8
constexpr uint32_t TMP_BYTES = 8192;
using DT = float;

class KernelLayerNormGrad {
public:
    __aicore__ inline KernelLayerNormGrad() {}
    __aicore__ inline void Init(GM_ADDR dy, GM_ADDR x, GM_ADDR variance, GM_ADDR mean,
                                GM_ADDR gamma, GM_ADDR pdX, GM_ADDR resGamma)
    {
        dyGm.SetGlobalBuffer((__gm__ DT *)dy, TOTAL);
        xGm.SetGlobalBuffer((__gm__ DT *)x, TOTAL);
        varGm.SetGlobalBuffer((__gm__ DT *)variance, BS_PAD);
        meanGm.SetGlobalBuffer((__gm__ DT *)mean, BS_PAD);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, H);
        pdXGm.SetGlobalBuffer((__gm__ DT *)pdX, TOTAL);
        resGammaGm.SetGlobalBuffer((__gm__ DT *)resGamma, TOTAL);
        pipe.InitBuffer(inDy, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inX, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inVar, 1, BS_PAD * sizeof(DT));
        pipe.InitBuffer(inMean, 1, BS_PAD * sizeof(DT));
        pipe.InitBuffer(inGamma, 1, H * sizeof(DT));
        pipe.InitBuffer(outPdX, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(outResGamma, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> dyL = inDy.AllocTensor<DT>();
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> vL = inVar.AllocTensor<DT>();
        AscendC::LocalTensor<DT> mL = inMean.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.AllocTensor<DT>();
        AscendC::DataCopy(dyL, dyGm, TOTAL);
        AscendC::DataCopy(xL, xGm, TOTAL);
        AscendC::DataCopy(vL, varGm, BS_PAD);
        AscendC::DataCopy(mL, meanGm, BS_PAD);
        AscendC::DataCopy(gL, gammaGm, H);
        inDy.EnQue(dyL); inX.EnQue(xL); inVar.EnQue(vL); inMean.EnQue(mL); inGamma.EnQue(gL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> dyL = inDy.DeQue<DT>();
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> vL = inVar.DeQue<DT>();
        AscendC::LocalTensor<DT> mL = inMean.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.DeQue<DT>();
        AscendC::LocalTensor<DT> pdxL = outPdX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> rgL = outResGamma.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::tiling::LayerNormGradTiling t;
        t.stackBufferSize = TMP_BYTES / sizeof(float);
        t.bLength = B;
        t.sLength = S;
        t.hLength = H;
        t.originalHLength = H;
        // 1/H and 2/H carried as float bit-patterns inside uint32 fields (impl reinterpret_casts them).
        float oneOverH = 1.0f / (float)H;
        float twoOverH = 2.0f / (float)H;
        t.lastDimValueBack = *reinterpret_cast<uint32_t *>(&oneOverH);
        t.lastDimValueBackMulTwo = *reinterpret_cast<uint32_t *>(&twoOverH);

        AscendC::LayerNormGrad<DT>(pdxL, rgL, dyL, xL, vL, mL, gL, tmp, (DT)1e-5f, t);

        outPdX.EnQue<DT>(pdxL);
        outResGamma.EnQue<DT>(rgL);
        inDy.FreeTensor(dyL); inX.FreeTensor(xL); inVar.FreeTensor(vL);
        inMean.FreeTensor(mL); inGamma.FreeTensor(gL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> pdxL = outPdX.DeQue<DT>();
        AscendC::LocalTensor<DT> rgL = outResGamma.DeQue<DT>();
        AscendC::DataCopy(pdXGm, pdxL, TOTAL);
        AscendC::DataCopy(resGammaGm, rgL, TOTAL);
        outPdX.FreeTensor(pdxL);
        outResGamma.FreeTensor(rgL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inDy, inX, inVar, inMean, inGamma;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outPdX, outResGamma;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> dyGm, xGm, varGm, meanGm, gammaGm, pdXGm, resGammaGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR dy, GM_ADDR x, GM_ADDR variance,
                                               GM_ADDR mean, GM_ADDR gamma, GM_ADDR pdX, GM_ADDR resGamma)
{
    KernelLayerNormGrad op;
    op.Init(dy, x, variance, mean, gamma, pdX, resGamma);
    op.Process();
}
