/*
 * Hand-written high-level unit: LayerNormGradBeta (adv_api/normalization, with LayerNormGradBetaTiling, this arch=3510).
 * Second stage of the LayerNorm backward: reduce over the (B,S) axis to obtain the gamma/beta gradients.
 *   pd_gamma[h] = sum_bs( inputDy[bs,h] * resForGamma[bs,h] )
 *   pd_beta[h]  = sum_bs( inputDy[bs,h] )
 * The 3510 impl only reads tiling.{bsLength,hLength}; it requires hLength*sizeof(T) % 32 == 0.
 *
 * Verifiable design [BS=4, H=8]: dy=2.0, resForGamma=3.0 (all constant).
 *   pd_gamma[h] = 4 * (2*3) = 24.0 ; pd_beta[h] = 4 * 2 = 8.0. Verified on host.
 */
#include "kernel_operator.h"
#include "lib/normalization/layernormgradbeta.h"

constexpr uint32_t BS = 4, H = 8;          // H*4=32B aligned
constexpr uint32_t TOTAL = BS * H;         // 32
constexpr uint32_t TMP_BYTES = 8192;
using DT = float;

class KernelLayerNormGradBeta {
public:
    __aicore__ inline KernelLayerNormGradBeta() {}
    __aicore__ inline void Init(GM_ADDR resGamma, GM_ADDR dy, GM_ADDR pdGamma, GM_ADDR pdBeta)
    {
        resGammaGm.SetGlobalBuffer((__gm__ DT *)resGamma, TOTAL);
        dyGm.SetGlobalBuffer((__gm__ DT *)dy, TOTAL);
        pdGammaGm.SetGlobalBuffer((__gm__ DT *)pdGamma, H);
        pdBetaGm.SetGlobalBuffer((__gm__ DT *)pdBeta, H);
        pipe.InitBuffer(inResGamma, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inDy, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(outPdGamma, 1, H * sizeof(DT));
        pipe.InitBuffer(outPdBeta, 1, H * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> rgL = inResGamma.AllocTensor<DT>();
        AscendC::LocalTensor<DT> dyL = inDy.AllocTensor<DT>();
        AscendC::DataCopy(rgL, resGammaGm, TOTAL);
        AscendC::DataCopy(dyL, dyGm, TOTAL);
        inResGamma.EnQue(rgL);
        inDy.EnQue(dyL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> rgL = inResGamma.DeQue<DT>();
        AscendC::LocalTensor<DT> dyL = inDy.DeQue<DT>();
        AscendC::LocalTensor<DT> pgL = outPdGamma.AllocTensor<DT>();
        AscendC::LocalTensor<DT> pbL = outPdBeta.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::tiling::LayerNormGradBetaTiling t;
        t.bsLength = BS;
        t.hLength = H;
        t.bshLength = TOTAL;
        t.originalHLength = H;
        t.stackBufferSize = TMP_BYTES / sizeof(float);

        AscendC::LayerNormGradBeta<DT>(pgL, pbL, rgL, dyL, tmp, t);

        outPdGamma.EnQue<DT>(pgL);
        outPdBeta.EnQue<DT>(pbL);
        inResGamma.FreeTensor(rgL);
        inDy.FreeTensor(dyL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> pgL = outPdGamma.DeQue<DT>();
        AscendC::LocalTensor<DT> pbL = outPdBeta.DeQue<DT>();
        AscendC::DataCopy(pdGammaGm, pgL, H);
        AscendC::DataCopy(pdBetaGm, pbL, H);
        outPdGamma.FreeTensor(pgL);
        outPdBeta.FreeTensor(pbL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inResGamma, inDy;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outPdGamma, outPdBeta;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> resGammaGm, dyGm, pdGammaGm, pdBetaGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR resGamma, GM_ADDR dy, GM_ADDR pdGamma, GM_ADDR pdBeta)
{
    KernelLayerNormGradBeta op;
    op.Init(resGamma, dy, pdGamma, pdBeta);
    op.Process();
}
