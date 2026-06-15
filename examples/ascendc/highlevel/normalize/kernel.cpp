/*
 * Hand-written high-level unit: Normalize (adv_api, this arch=3510).
 * Given precomputed per-row mean and variance, Normalize computes:
 *   rstd = rsqrt(variance + epsilon)
 *   output = (x - mean) * rstd * gamma + beta
 * Shapes: output/inputX [A, R], outputRstd/inputMean/inputVariance [A], gamma/beta [R].
 * The 3510 impl reads only para.{aLength, rLength, rLengthWithPadding}; no host tiling is needed.
 * Design: mean=0, variance=1, eps=0 -> rstd=1; x=1, gamma=1, beta=0 -> output=(1-0)*1*1+0=1, rstd=1.
 */
#include "kernel_operator.h"
#include "lib/normalization/normalize.h"

constexpr uint32_t A = 2;                // number of rows (aLength); halfA=1, no tail row
constexpr uint32_t APAD = 8;            // [A]-shaped tensors padded to 32B for DataCopy (min 8 floats)
constexpr uint32_t R = 8;               // reduce length (< 64 -> single repeat)
constexpr uint32_t RPAD = 8;            // rLengthWithPadding, 32B aligned (8*4=32)
constexpr uint32_t TOTAL = A * RPAD;    // 16
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelNormalize {
public:
    __aicore__ inline KernelNormalize() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR mean, GM_ADDR var, GM_ADDR gamma, GM_ADDR beta,
                                GM_ADDR y, GM_ADDR rstd)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, TOTAL);
        meanGm.SetGlobalBuffer((__gm__ DT *)mean, APAD);
        varGm.SetGlobalBuffer((__gm__ DT *)var, APAD);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, R);
        betaGm.SetGlobalBuffer((__gm__ DT *)beta, R);
        yGm.SetGlobalBuffer((__gm__ DT *)y, TOTAL);
        rstdGm.SetGlobalBuffer((__gm__ DT *)rstd, APAD);

        pipe.InitBuffer(inQueueX, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inQueueM, 1, APAD * sizeof(DT));
        pipe.InitBuffer(inQueueV, 1, APAD * sizeof(DT));
        pipe.InitBuffer(inQueueG, 1, R * sizeof(DT));
        pipe.InitBuffer(inQueueB, 1, R * sizeof(DT));
        pipe.InitBuffer(outQueueY, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(outQueueR, 1, APAD * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> mL = inQueueM.AllocTensor<DT>();
        AscendC::LocalTensor<DT> vL = inQueueV.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, TOTAL);
        AscendC::DataCopy(mL, meanGm, APAD);
        AscendC::DataCopy(vL, varGm, APAD);
        AscendC::DataCopy(gL, gammaGm, R);
        AscendC::DataCopy(bL, betaGm, R);
        inQueueX.EnQue(xL); inQueueM.EnQue(mL); inQueueV.EnQue(vL);
        inQueueG.EnQue(gL); inQueueB.EnQue(bL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> mL = inQueueM.DeQue<DT>();
        AscendC::LocalTensor<DT> vL = inQueueV.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.DeQue<DT>();
        AscendC::LocalTensor<DT> yL = outQueueY.AllocTensor<DT>();
        AscendC::LocalTensor<float> rstdL = outQueueR.AllocTensor<float>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::NormalizePara para;
        para.aLength = A;
        para.rLength = R;
        para.rLengthWithPadding = RPAD;

        // U = T = float; isReuseSource=false; default NormalizeConfig (AR pattern, with gamma and beta).
        AscendC::Normalize<DT, DT>(yL, rstdL, mL, vL, xL, gL, bL, tmp, (float)0.0f, para);

        outQueueY.EnQue<DT>(yL);
        outQueueR.EnQue<float>(rstdL);
        inQueueX.FreeTensor(xL);
        inQueueM.FreeTensor(mL);
        inQueueV.FreeTensor(vL);
        inQueueG.FreeTensor(gL);
        inQueueB.FreeTensor(bL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> yL = outQueueY.DeQue<DT>();
        AscendC::LocalTensor<float> rstdL = outQueueR.DeQue<float>();
        AscendC::DataCopy(yGm, yL, TOTAL);
        AscendC::DataCopy(rstdGm, rstdL, APAD);
        outQueueY.FreeTensor(yL);
        outQueueR.FreeTensor(rstdL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueM, inQueueV, inQueueG, inQueueB;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueY, outQueueR;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, meanGm, varGm, gammaGm, betaGm, yGm, rstdGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR mean, GM_ADDR var, GM_ADDR gamma,
                                               GM_ADDR beta, GM_ADDR y, GM_ADDR rstd)
{
    KernelNormalize op;
    op.Init(x, mean, var, gamma, beta, y, rstd);
    op.Process();
}
