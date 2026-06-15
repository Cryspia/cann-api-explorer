/*
 * Hand-written high-level unit: SoftmaxFlashV3 (adv_api, FlashAttention-2 online softmax, with Tiling).
 * Semantics (from softmaxflashv3.h / 3510 impl, isUpdate = false, first block):
 *     rowMeanGlobal = rowsum(x) / K            (per-row)
 *     mean          = rowMeanGlobal
 *     x'            = x - meanTmp * alpha/(1-alpha)   (meanTmp derived from the split-mean scheme)
 *     max           = rowmax(x')
 *     y             = exp(x' - max)             (un-normalized, NOT divided by sum)
 *     sum           = rowsum(y)
 * With x all-zero every per-row statistic collapses to 0, so x'=0, mean=0, max=0,
 * y=exp(0)=1.0, sum=K=64. Shape [M,K]=[8,64], reduction along K.
 *
 * Data types are fixed by the API: T (src/dst/expMax) = half, U (mean/sum/max) = float.
 * Tiling is built on the device side via AscendC::SoftMaxTilingFunc (same SoftMaxTiling as
 * SoftMax); the FlashV3 specific knobs live in SoftMaxParams. With K=64 the vector repeat
 * stride is 64 floats, so kRepeatTime=1; splitMeanCnt is set to 1 so that
 * remainRepeatTime = kRepeatTime - splitMeanCnt = 0 and baseK = K (single split).
 *
 * Reduce outputs (mean, sum, max) follow the AscendC block layout: each row owns one 32B
 * block (8 floats), the reduced scalar lives in column 0 of that block; sized M * 8.
 */
#include "kernel_operator.h"
#include "lib/activation/softmaxflashv3.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t BLK_F32 = 8;                // 32B / sizeof(float)
constexpr uint32_t RED = M * BLK_F32;          // 64, reduce-tensor element count
constexpr uint32_t TMP_BYTES = 32768;          // ample temporary space
using T = half;                                // src / dst / expMax
using U = float;                               // mean / sum / max

class KernelSoftmaxFlashV3 {
public:
    __aicore__ inline KernelSoftmaxFlashV3() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z, GM_ADDR meanOut, GM_ADDR sumOut, GM_ADDR maxOut)
    {
        xGm.SetGlobalBuffer((__gm__ T *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ T *)z, ELEM);
        meanGm.SetGlobalBuffer((__gm__ U *)meanOut, RED);
        sumGm.SetGlobalBuffer((__gm__ U *)sumOut, RED);
        maxGm.SetGlobalBuffer((__gm__ U *)maxOut, RED);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(T));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(T));
        pipe.InitBuffer(outQueueMean, 1, RED * sizeof(U));
        pipe.InitBuffer(outQueueSum, 1, RED * sizeof(U));
        pipe.InitBuffer(outQueueMax, 1, RED * sizeof(U));
        pipe.InitBuffer(expMaxBuf, RED * sizeof(T));
        pipe.InitBuffer(inMeanBuf, RED * sizeof(U));
        pipe.InitBuffer(inSumBuf, RED * sizeof(U));
        pipe.InitBuffer(inMaxBuf, RED * sizeof(U));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<T> xLocal = inQueueX.AllocTensor<T>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<T> xLocal = inQueueX.DeQue<T>();
        AscendC::LocalTensor<T> zLocal = outQueueZ.AllocTensor<T>();
        AscendC::LocalTensor<U> meanLocal = outQueueMean.AllocTensor<U>();
        AscendC::LocalTensor<U> sumLocal = outQueueSum.AllocTensor<U>();
        AscendC::LocalTensor<U> maxLocal = outQueueMax.AllocTensor<U>();
        AscendC::LocalTensor<T> expMaxLocal = expMaxBuf.Get<T>();
        AscendC::LocalTensor<U> inMeanLocal = inMeanBuf.Get<U>();
        AscendC::LocalTensor<U> inSumLocal = inSumBuf.Get<U>();
        AscendC::LocalTensor<U> inMaxLocal = inMaxBuf.Get<U>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        AscendC::tiling::SoftMaxTiling tiling;
        // Build the SoftMaxTiling on the device side (dataTypeSize1 = src/dst size,
        // dataTypeSize2 = reduce-output size).
        AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(float), info, tiling,
                                   sizeof(T), sizeof(U));

        // FlashV3 knobs: single mean split so that K=64 (kRepeatTime=1) is well-formed.
        AscendC::SoftMaxParams params;
        params.srcM = M;
        params.srcK = K;
        params.oriSrcM = M;
        params.oriSrcK = K;
        params.splitMeanCnt = 1;

        // isUpdate = false: inMean / inSum / inMax / expMax are unused (first block).
        AscendC::SoftmaxFlashV3<T, U, false>(zLocal, meanLocal, sumLocal, maxLocal, xLocal,
                                             expMaxLocal, inMeanLocal, inSumLocal, inMaxLocal,
                                             tmp, tiling, params);

        outQueueZ.EnQue<T>(zLocal);
        outQueueMean.EnQue<U>(meanLocal);
        outQueueSum.EnQue<U>(sumLocal);
        outQueueMax.EnQue<U>(maxLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<T> zLocal = outQueueZ.DeQue<T>();
        AscendC::LocalTensor<U> meanLocal = outQueueMean.DeQue<U>();
        AscendC::LocalTensor<U> sumLocal = outQueueSum.DeQue<U>();
        AscendC::LocalTensor<U> maxLocal = outQueueMax.DeQue<U>();
        AscendC::DataCopy(zGm, zLocal, ELEM);
        AscendC::DataCopy(meanGm, meanLocal, RED);
        AscendC::DataCopy(sumGm, sumLocal, RED);
        AscendC::DataCopy(maxGm, maxLocal, RED);
        outQueueZ.FreeTensor(zLocal);
        outQueueMean.FreeTensor(meanLocal);
        outQueueSum.FreeTensor(sumLocal);
        outQueueMax.FreeTensor(maxLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueMean;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueSum;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueMax;
    AscendC::TBuf<AscendC::TPosition::VECCALC> expMaxBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> inMeanBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> inSumBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> inMaxBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<T> xGm, zGm;
    AscendC::GlobalTensor<U> meanGm, sumGm, maxGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z, GM_ADDR meanOut,
                                               GM_ADDR sumOut, GM_ADDR maxOut)
{
    KernelSoftmaxFlashV3 op;
    op.Init(x, z, meanOut, sumOut, maxOut);
    op.Process();
}
