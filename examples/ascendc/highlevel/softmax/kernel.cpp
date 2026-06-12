/*
 * Hand-written high-level unit: SoftMax (adv_api, with Tiling).
 * Key point: use the device-side AscendC::SoftMaxTilingFunc to construct SoftMaxTiling inside the kernel,
 * avoiding the host tiling framework. Shape [M,K]=[8,64], softmax along the last dimension K.
 */
#include "kernel_operator.h"
#include "lib/activation/softmax.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t TMP_BYTES = 16384;          // ample temporary space
using DT = float;

class KernelSoftmax {
public:
    __aicore__ inline KernelSoftmax() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
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
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        AscendC::tiling::SoftMaxTiling tiling;
        // Construct tiling on the device side: workLocalSize is counted in B32 elements
        AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(DT), info, tiling,
                                   sizeof(DT), sizeof(DT));
        AscendC::SoftMax<DT>(zLocal, xLocal, tmp, tiling, info);

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
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelSoftmax op;
    op.Init(x, z);
    op.Process();
}
