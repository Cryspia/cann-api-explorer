/*
 * Hand-written high-level unit: SimpleSoftMax (adv_api, with Tiling).
 * Semantics (from simplesoftmax.h / 3510 impl):
 *     y = exp(x - inMax) / inSum
 * It is the "simplified" softmax that takes pre-computed per-row max and sum, so it only
 * performs the final normalization step. Shape [M,K]=[8,64], reduction along K.
 * Tiling is built on the device side via AscendC::SoftMaxTilingFunc, mirroring the softmax
 * unit, avoiding the host tiling framework.
 *
 * inMax / inSum follow the AscendC reduce block layout: each row owns one 32B block
 * (8 floats), the per-row scalar lives in column 0 of that block. So they are sized
 * M * (32 / sizeof(float)) = M * 8 elements.
 */
#include "kernel_operator.h"
#include "lib/activation/simplesoftmax.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t BLK_F32 = 8;                // 32B / sizeof(float)
constexpr uint32_t RED = M * BLK_F32;          // 64, reduce-tensor element count
constexpr uint32_t TMP_BYTES = 16384;          // ample temporary space
using DT = float;

class KernelSimpleSoftMax {
public:
    __aicore__ inline KernelSimpleSoftMax() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR inSum, GM_ADDR inMax, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        inSumGm.SetGlobalBuffer((__gm__ DT *)inSum, RED);
        inMaxGm.SetGlobalBuffer((__gm__ DT *)inMax, RED);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueSum, 1, RED * sizeof(DT));
        pipe.InitBuffer(inQueueMax, 1, RED * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> sumLocal = inQueueSum.AllocTensor<DT>();
        AscendC::LocalTensor<DT> maxLocal = inQueueMax.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        AscendC::DataCopy(sumLocal, inSumGm, RED);
        AscendC::DataCopy(maxLocal, inMaxGm, RED);
        inQueueX.EnQue(xLocal);
        inQueueSum.EnQue(sumLocal);
        inQueueMax.EnQue(maxLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> sumLocal = inQueueSum.DeQue<DT>();
        AscendC::LocalTensor<DT> maxLocal = inQueueMax.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        AscendC::tiling::SoftMaxTiling tiling;
        // Construct tiling on the device side: workLocalSize is counted in B32 elements.
        AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(DT), info, tiling,
                                   sizeof(DT), sizeof(DT));
        // y = exp(x - inMax) / inSum
        AscendC::SimpleSoftMax<DT>(zLocal, sumLocal, maxLocal, xLocal, tmp, tiling, info);

        outQueueZ.EnQue<DT>(zLocal);
        inQueueX.FreeTensor(xLocal);
        inQueueSum.FreeTensor(sumLocal);
        inQueueMax.FreeTensor(maxLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, ELEM);
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSum;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueMax;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, inSumGm, inMaxGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR inSum, GM_ADDR inMax, GM_ADDR z)
{
    KernelSimpleSoftMax op;
    op.Init(x, inSum, inMax, z);
    op.Process();
}
