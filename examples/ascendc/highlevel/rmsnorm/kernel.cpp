/*
 * Hand-written high-level unit: RmsNorm (adv_api, with Tiling).
 * RmsNorm has no device-side tiling func, but RmsNormTiling has only 12 fields;
 * with a single-tile fixed shape [B,S,H]=[1,8,64] it can be hand-filled inside the kernel (loopRound=1, no tail).
 * y = x / sqrt(mean(x^2)+eps) * gamma.
 */
#include "kernel_operator.h"
#include "lib/normalization/rmsnorm.h"

constexpr uint32_t B = 1;
constexpr uint32_t S = 8;
constexpr uint32_t H = 64;
constexpr uint32_t BSH = B * S * H;            // 512
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelRmsNorm {
public:
    __aicore__ inline KernelRmsNorm() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR gamma, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, BSH);
        gammaGm.SetGlobalBuffer((__gm__ DT *)gamma, H);
        zGm.SetGlobalBuffer((__gm__ DT *)z, BSH);
        pipe.InitBuffer(inQueueX, 1, BSH * sizeof(DT));
        pipe.InitBuffer(inQueueG, 1, H * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, BSH * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, BSH);
        AscendC::DataCopy(gL, gammaGm, H);
        inQueueX.EnQue(xL);
        inQueueG.EnQue(gL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> gL = inQueueG.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        // Single-tile hand-filled tiling: everything in one loop, no tail
        AscendC::tiling::RmsNormTiling t;
        t.bLength = B;
        t.sLength = S;
        t.hLength = H;
        t.originalHLength = H;
        t.reciprocalOfHLength = 1.0f / (float)H;
        t.mainBshLength = BSH;
        t.mainBsLength = B * S;
        t.mainBsLengthAlign = B * S;       // B*S=8 is already 32B/float aligned
        t.loopRound = 1;
        t.inputTailPos = 0;
        t.tailBshLength = 0;
        t.tailBsLength = 0;

        AscendC::RmsNorm<DT>(zL, xL, gL, tmp, (DT)1e-5f, t);

        outQueueZ.EnQue<DT>(zL);
        inQueueX.FreeTensor(xL);
        inQueueG.FreeTensor(gL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, BSH);
        outQueueZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueG;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, gammaGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR gamma, GM_ADDR z)
{
    KernelRmsNorm op;
    op.Init(x, gamma, z);
    op.Process();
}
