/*
 * Hand-written high-level unit: LogSoftMax (adv_api, with Tiling).
 * compute: max=rowmax(x), sum=rowsum(exp(x-max)), y=log(exp(x-max)/sum)=log(softmax(x)).
 * Key: LogSoftMaxTiling has exactly the same struct layout as SoftMaxTiling (16 uint32 fields),
 * so reuse the device-side AscendC::SoftMaxTilingFunc to fill SoftMaxTiling, then copy field by field into LogSoftMaxTiling, avoiding host tiling.
 * Shape [M,K]=[8,64], x all 0 -> softmax=1/64 -> logsoftmax=log(1/64)=-ln(64)~=-4.158883.
 */
#include "kernel_operator.h"
#include "lib/activation/softmax.h"
#include "lib/activation/logsoftmax.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t RED = M * 8;                // spare space for sum/max (reduce dim aligned to 8)
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelLogSoftmax {
public:
    __aicore__ inline KernelLogSoftmax() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(sumBuf, RED * sizeof(DT));
        pipe.InitBuffer(maxBuf, RED * sizeof(DT));
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
        AscendC::LocalTensor<DT> sumLocal = sumBuf.Get<DT>();
        AscendC::LocalTensor<DT> maxLocal = maxBuf.Get<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        // reuse device SoftMaxTilingFunc to compute SoftMaxTiling
        AscendC::tiling::SoftMaxTiling smt;
        AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(DT), info, smt, sizeof(DT), sizeof(DT));
        // LogSoftMaxTiling has the same layout as SoftMaxTiling, copy field by field
        AscendC::tiling::LogSoftMaxTiling lst;
        auto *s = reinterpret_cast<uint32_t *>(&smt);
        auto *d = reinterpret_cast<uint32_t *>(&lst);
        for (uint32_t i = 0; i < sizeof(AscendC::tiling::LogSoftMaxTiling) / sizeof(uint32_t); i++) d[i] = s[i];

        AscendC::LogSoftMax<DT>(zLocal, sumLocal, maxLocal, xLocal, tmp, lst, info);

        outQueueZ.EnQue<DT>(zLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, ELEM);
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> sumBuf, maxBuf, tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelLogSoftmax op;
    op.Init(x, z);
    op.Process();
}
