/*
 * Hand-written high-level unit: SoftmaxFlashV2 (adv_api, FlashAttention-2 online softmax, with Tiling).
 * Semantics (from softmaxflashv2.h, isUpdate = false, first block):
 *     max = rowmax(x)
 *     y   = exp(x - max)          // NOT divided by sum
 *     sum = rowsum(y)
 * Shape [M,K]=[8,64], reduction along K. Tiling is built on the device side via the
 * device-callable AscendC::SoftMaxFlashV2TilingFunc, avoiding the host tiling framework.
 *
 * Reduce outputs (sum, max) follow the AscendC block layout: each row owns one 32B block
 * (8 floats); the reduced scalar lives in column 0 of that block. So they are sized
 * M * (32 / sizeof(float)) = M * 8 elements.
 */
#include "kernel_operator.h"
#include "lib/activation/softmaxflashv2.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t BLK_F32 = 8;                // 32B / sizeof(float)
constexpr uint32_t RED = M * BLK_F32;          // 64, reduce-tensor element count
constexpr uint32_t TMP_BYTES = 16384;          // ample temporary space
using DT = float;

class KernelSoftmaxFlashV2 {
public:
    __aicore__ inline KernelSoftmaxFlashV2() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z, GM_ADDR sumOut, GM_ADDR maxOut)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        sumGm.SetGlobalBuffer((__gm__ DT *)sumOut, RED);
        maxGm.SetGlobalBuffer((__gm__ DT *)maxOut, RED);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueSum, 1, RED * sizeof(DT));
        pipe.InitBuffer(outQueueMax, 1, RED * sizeof(DT));
        pipe.InitBuffer(expMaxBuf, RED * sizeof(DT));
        pipe.InitBuffer(inSumBuf, RED * sizeof(DT));
        pipe.InitBuffer(inMaxBuf, RED * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<DT> sumLocal = outQueueSum.AllocTensor<DT>();
        AscendC::LocalTensor<DT> maxLocal = outQueueMax.AllocTensor<DT>();
        AscendC::LocalTensor<DT> expMaxLocal = expMaxBuf.Get<DT>();
        AscendC::LocalTensor<DT> inSumLocal = inSumBuf.Get<DT>();
        AscendC::LocalTensor<DT> inMaxLocal = inMaxBuf.Get<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        // Build the FlashV2 tiling on the device side (constexpr helper from the header).
        // dataTypeSize1 = src/dst dtype size, dataTypeSize2 = sum/max dtype size.
        auto tiling =
            AscendC::SoftMaxFlashV2TilingFunc(info, sizeof(DT), sizeof(DT), TMP_BYTES, false);

        // isUpdate = false: inSum / inMax / expMax are unused (first block).
        AscendC::SoftmaxFlashV2<DT, false>(zLocal, sumLocal, maxLocal, xLocal, expMaxLocal,
                                           inSumLocal, inMaxLocal, tmp, tiling, info);

        outQueueZ.EnQue<DT>(zLocal);
        outQueueSum.EnQue<DT>(sumLocal);
        outQueueMax.EnQue<DT>(maxLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::LocalTensor<DT> sumLocal = outQueueSum.DeQue<DT>();
        AscendC::LocalTensor<DT> maxLocal = outQueueMax.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, ELEM);
        AscendC::DataCopy(sumGm, sumLocal, RED);
        AscendC::DataCopy(maxGm, maxLocal, RED);
        outQueueZ.FreeTensor(zLocal);
        outQueueSum.FreeTensor(sumLocal);
        outQueueMax.FreeTensor(maxLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueSum;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueMax;
    AscendC::TBuf<AscendC::TPosition::VECCALC> expMaxBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> inSumBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> inMaxBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm, sumGm, maxGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z, GM_ADDR sumOut, GM_ADDR maxOut)
{
    KernelSoftmaxFlashV2 op;
    op.Init(x, z, sumOut, maxOut);
    op.Process();
}
