/*
 * Hand-written high-level unit: LayerNorm (adv_api, this arch=3510 takes the regbase path).
 * 3510 only provides the public API LayerNorm(Para, LayerNormSeparateTiling) (not the standard LayerNormTiling).
 * Take the simple branch with rLength=64 (<=sregLower): only para.aLength/rLength/rLengthWithPadding
 * + tiling.rLength/k2Rec/k2RRec (=1/R) are needed, no tmp layout fields, can be hand-filled inside the kernel.
 * y = (x-mean)/sqrt(var+eps)*gamma + beta.
 */
#include "kernel_operator.h"
#include "lib/normalization/layernorm.h"

constexpr uint32_t A = 8;     // number of rows
constexpr uint32_t R = 64;    // normalization dimension (last dim)
constexpr uint32_t AR = A * R;
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelLayerNorm {
public:
    __aicore__ inline KernelLayerNorm() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, AR);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, R);
        betaGm.SetGlobalBuffer((__gm__ DT *)beta, R);
        zGm.SetGlobalBuffer((__gm__ DT *)z, AR);
        pipe.InitBuffer(inQueueX, 1, AR * sizeof(DT));
        pipe.InitBuffer(inQueueG, 1, R * sizeof(DT));
        pipe.InitBuffer(inQueueB, 1, R * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, AR * sizeof(DT));
        pipe.InitBuffer(meanBuf, A * sizeof(float) + 32);
        pipe.InitBuffer(rstdBuf, A * sizeof(float) + 32);
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, AR);
        AscendC::DataCopy(gL, gammaGm, R);
        AscendC::DataCopy(bL, betaGm, R);
        inQueueX.EnQue(xL); inQueueG.EnQue(gL); inQueueB.EnQue(bL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<float> meanL = meanBuf.Get<float>();
        AscendC::LocalTensor<float> rstdL = rstdBuf.Get<float>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::LayerNormPara para;
        para.aLength = A;
        para.rLength = R;
        para.rLengthWithPadding = R;            // R=64 is already 64-aligned

        AscendC::tiling::LayerNormSeparateTiling t;
        t.aLength = A;
        t.rLength = R;
        // mean = k2RRec · ReduceSum(k2Rec · x) => k2Rec·k2RRec must = 1/R
        t.k2Rec = 1.0f;                         // per-element coefficient
        t.k2RRec = 1.0f / (float)R;             // divide by R after reduce

        AscendC::LayerNorm<DT, DT>(zL, meanL, rstdL, xL, gL, bL, (float)1e-5f, tmp, para, t);

        outQueueZ.EnQue<DT>(zL);
        inQueueX.FreeTensor(xL);
        inQueueG.FreeTensor(gL);
        inQueueB.FreeTensor(bL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, AR);
        outQueueZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueG, inQueueB;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> meanBuf, rstdBuf, tmpBuf;
    AscendC::GlobalTensor<DT> xGm, gammaGm, betaGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta, GM_ADDR z)
{
    KernelLayerNorm op;
    op.Init(x, gamma, beta, z);
    op.Process();
}
