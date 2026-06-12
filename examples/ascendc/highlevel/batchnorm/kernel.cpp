/*
 * Hand-written high-level unit: BatchNorm (adv_api/normalization, with BatchNormTiling; 3510 needs only 3 fields).
 * Formula: output = gamma * (x - mean) * rsqrt(var + eps) + beta, mean = sum_b(x)/B (normalized along first dim B).
 * Memory layout [B, F] row-major: srcLocal[b*F + f]. The 3510 impl only reads originalBLength / meanVarSize / firstDimValueBack.
 * firstDimValueBack = 1/B (mean coefficient, filled in directly, no back-derivation needed).
 * Design [B=4,F=8], x all 5 -> the 4 batch values per feature are equal -> var=0 -> output = beta.
 * gamma=1, beta=2 -> output all 2, mean all 5.
 */
#include "kernel_operator.h"
#include "lib/normalization/batchnorm.h"

constexpr uint32_t B = 4, F = 8;   // F*4=32B aligned
constexpr uint32_t BF = B * F;
using DT = float;

class KernelBatchNorm {
public:
    __aicore__ inline KernelBatchNorm() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta, GM_ADDR z, GM_ADDR mean, GM_ADDR var)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, BF);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, F);
        betaGm.SetGlobalBuffer((__gm__ DT *)beta, F);
        zGm.SetGlobalBuffer((__gm__ DT *)z, BF);
        meanGm.SetGlobalBuffer((__gm__ DT *)mean, F);
        varGm.SetGlobalBuffer((__gm__ DT *)var, F);
        pipe.InitBuffer(inX, 1, BF * sizeof(DT));
        pipe.InitBuffer(inGamma, 1, F * sizeof(DT));
        pipe.InitBuffer(inBeta, 1, F * sizeof(DT));
        pipe.InitBuffer(outZ, 1, BF * sizeof(DT));
        pipe.InitBuffer(outMean, 1, F * sizeof(DT));
        pipe.InitBuffer(outVar, 1, F * sizeof(DT));
        pipe.InitBuffer(tmpBuf, 4096);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inBeta.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, BF);
        AscendC::DataCopy(gL, gammaGm, F);
        AscendC::DataCopy(bL, betaGm, F);
        inX.EnQue(xL); inGamma.EnQue(gL); inBeta.EnQue(bL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inGamma.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inBeta.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<DT> meanL = outMean.AllocTensor<DT>();
        AscendC::LocalTensor<DT> varL = outVar.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::tiling::BatchNormTiling t;
        t.originalBLength = B;
        t.meanVarSize = F;
        t.firstDimValueBack = 1.0f / (float)B;

        const DT eps = 1e-5f;
        AscendC::BatchNorm<DT, false, false>(zL, meanL, varL, xL, gL, bL, tmp, eps, t);

        outZ.EnQue<DT>(zL); outMean.EnQue<DT>(meanL); outVar.EnQue<DT>(varL);
        inX.FreeTensor(xL); inGamma.FreeTensor(gL); inBeta.FreeTensor(bL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::LocalTensor<DT> meanL = outMean.DeQue<DT>();
        AscendC::LocalTensor<DT> varL = outVar.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, BF);
        AscendC::DataCopy(meanGm, meanL, F);
        AscendC::DataCopy(varGm, varL, F);
        outZ.FreeTensor(zL); outMean.FreeTensor(meanL); outVar.FreeTensor(varL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inGamma, inBeta;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ, outMean, outVar;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, gammaGm, betaGm, zGm, meanGm, varGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta,
                                               GM_ADDR z, GM_ADDR mean, GM_ADDR var)
{
    KernelBatchNorm op;
    op.Init(x, gamma, beta, z, mean, var);
    op.Process();
}
