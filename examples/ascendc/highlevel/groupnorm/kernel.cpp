/*
 * Hand-written high-level unit: GroupNorm (adv_api, with GroupNormTiling, this arch=3510).
 * Layout [N, C, H*W], C=G*D, normalized over D*HW elements per (N,G) group.
 * The 3510 impl directly uses tiling.{n,g,d,hw,factor}; dhwAlignSize only selects a branch; oneTmpSize only needs >0.
 * factor=1/(D*HW) is the mean coefficient. Identical inputs -> in-group var=0 -> output=beta=0.
 */
#include "kernel_operator.h"
#include "lib/normalization/groupnorm.h"

constexpr uint32_t N = 1;
constexpr uint32_t G = 2;
constexpr uint32_t D = 2;
constexpr uint32_t HW = 16;
constexpr uint32_t C = G * D;            // 4
constexpr uint32_t TOTAL = N * C * HW;   // 64
constexpr uint32_t NG = N * G;           // 2 (number of mean/var)
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelGroupNorm {
public:
    __aicore__ inline KernelGroupNorm() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, TOTAL);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, C);
        betaGm.SetGlobalBuffer((__gm__ DT *)beta, C);
        zGm.SetGlobalBuffer((__gm__ DT *)z, TOTAL);
        pipe.InitBuffer(inQueueX, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inQueueG, 1, C * sizeof(DT));
        pipe.InitBuffer(inQueueB, 1, C * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(meanBuf, NG * sizeof(DT) + 32);
        pipe.InitBuffer(varBuf, NG * sizeof(DT) + 32);
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.AllocTensor<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, TOTAL);
        AscendC::DataCopy(gL, gammaGm, C);
        AscendC::DataCopy(bL, betaGm, C);
        inQueueX.EnQue(xL); inQueueG.EnQue(gL); inQueueB.EnQue(bL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.DeQue<DT>();
        AscendC::LocalTensor<DT> bL = inQueueB.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<DT> meanL = meanBuf.Get<DT>();
        AscendC::LocalTensor<DT> varL = varBuf.Get<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::tiling::GroupNormTiling t;
        t.n = N; t.c = C; t.g = G; t.d = D; t.hw = HW;
        t.dhwAlignSize = D * HW;            // 32, <= oneRepSize -> ShapeScope ONE
        t.factor = 1.0f / (float)(D * HW);  // mean coefficient 1/(D*HW)
        t.oneTmpSize = 256;                 // only needs > 0

        AscendC::GroupNorm<DT>(zL, meanL, varL, xL, gL, bL, tmp, (DT)1e-5f, t);

        outQueueZ.EnQue<DT>(zL);
        inQueueX.FreeTensor(xL);
        inQueueG.FreeTensor(gL);
        inQueueB.FreeTensor(bL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, TOTAL);
        outQueueZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueG, inQueueB;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> meanBuf, varBuf, tmpBuf;
    AscendC::GlobalTensor<DT> xGm, gammaGm, betaGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR gamma, GM_ADDR beta, GM_ADDR z)
{
    KernelGroupNorm op;
    op.Init(x, gamma, beta, z);
    op.Process();
}
