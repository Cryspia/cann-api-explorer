/*
 * Hand-written high-level unit: SoftmaxGradFront (adv_api, with Tiling).
 * Semantics (from softmaxgrad.h / 3510 impl):
 *     y = rowsum(grad * x)
 * This is the "Front" variant of the softmax backward: it only produces the per-row
 * reduction sum = rowsum(grad * x) (the term later reused by the full gradient), not the
 * full grad * x - sum * x output. Shape [M,K]=[8,64], reduction along K.
 * Tiling is built on the device side via AscendC::SoftMaxTilingFunc, mirroring the
 * softmaxgrad unit, avoiding the host tiling framework.
 *
 * The output is reduce-shaped: each row writes one scalar into column 0 of its own 32B
 * block (8 floats), so dst is sized M * (32 / sizeof(float)) = M * 8 elements.
 */
#include "kernel_operator.h"
#include "lib/activation/softmaxgrad.h"

constexpr uint32_t M = 8;
constexpr uint32_t K = 64;
constexpr uint32_t ELEM = M * K;               // 512
constexpr uint32_t BLK_F32 = 8;                // 32B / sizeof(float)
constexpr uint32_t RED = M * BLK_F32;          // 64, reduce-tensor element count
constexpr uint32_t TMP_BYTES = 16384;          // ample temporary space
using DT = float;

class KernelSoftmaxGradFront {
public:
    __aicore__ inline KernelSoftmaxGradFront() {}
    __aicore__ inline void Init(GM_ADDR grad, GM_ADDR x, GM_ADDR z)
    {
        gradGm.SetGlobalBuffer((__gm__ DT *)grad, ELEM);
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, RED);
        pipe.InitBuffer(inQueueGrad, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, RED * sizeof(DT));
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
        // y = rowsum(grad * x), one scalar per row in column 0 of its 32B block.
        AscendC::SoftmaxGradFront<DT>(zLocal, gradLocal, xLocal, tmp, tiling, info);

        outQueueZ.EnQue<DT>(zLocal);
        inQueueGrad.FreeTensor(gradLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, RED);
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
    KernelSoftmaxGradFront op;
    op.Init(grad, x, z);
    op.Process();
}
