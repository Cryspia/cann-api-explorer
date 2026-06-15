/*
 * Hand-written high-level unit: SoftmaxGrad (adv_api, with Tiling).
 * Semantics (from softmaxgrad.h, isFront = false):
 *     sum = rowsum(grad * x)
 *     y   = grad * x - sum * x
 * where x is the forward softmax output. Shape [M,K]=[8,64], reduction along K.
 * Tiling is built on the device side via AscendC::SoftMaxTilingFunc, mirroring the
 * softmax unit, avoiding the host tiling framework.
 */
#include "kernel_operator.h"
#include "lib/activation/softmaxgrad.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t TMP_BYTES = 16384;          // ample temporary space
using DT = float;

class KernelSoftmaxGrad {
public:
    __aicore__ inline KernelSoftmaxGrad() {}
    __aicore__ inline void Init(GM_ADDR grad, GM_ADDR x, GM_ADDR z)
    {
        gradGm.SetGlobalBuffer((__gm__ DT *)grad, ELEM);
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueGrad, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> gradLocal = inQueueGrad.AllocTensor<DT>();
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(gradLocal, gradGm, ELEM);
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueGrad.EnQue(gradLocal);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> gradLocal = inQueueGrad.DeQue<DT>();
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SoftMaxShapeInfo info{M, K, M, K};
        AscendC::tiling::SoftMaxTiling tiling;
        // Construct tiling on the device side: workLocalSize is counted in B32 elements.
        AscendC::SoftMaxTilingFunc(TMP_BYTES / sizeof(DT), info, tiling,
                                   sizeof(DT), sizeof(DT));
        // isFront = false -> y = grad * x - rowsum(grad * x) * x
        AscendC::SoftmaxGrad<DT>(zLocal, gradLocal, xLocal, tmp, tiling, false, info);

        outQueueZ.EnQue<DT>(zLocal);
        inQueueGrad.FreeTensor(gradLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, ELEM);
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueGrad;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> gradGm, xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR grad, GM_ADDR x, GM_ADDR z)
{
    KernelSoftmaxGrad op;
    op.Init(grad, x, z);
    op.Process();
}
