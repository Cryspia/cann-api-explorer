/*
 * Hand-written high-level unit: AscendDequant (adv_api quantization).
 * Formula: dst(i) = src(i) * deqScale(i % deqScale.GetSize()).
 * src is int32; deqScale is a per-channel LocalTensor (here filled with a constant).
 * Shape: a single block of ELEM elements; deqScale has the same length so each
 * element multiplies by the same scale.
 */
#include "kernel_operator.h"
#include "lib/quantization/ascend_dequant.h"

constexpr uint32_t ELEM = 256;                 // element count
constexpr uint32_t TMP_BYTES = 8192;           // ample shared temporary space
using DT = float;                              // dequantized output type
using ScaleT = float;                          // deqScale type

class KernelAscendDequant {
public:
    __aicore__ inline KernelAscendDequant() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ int32_t *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(int32_t));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(scaleBuf, ELEM * sizeof(ScaleT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<int32_t> xLocal = inQueueX.AllocTensor<int32_t>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<int32_t> xLocal = inQueueX.DeQue<int32_t>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<ScaleT> deqScale = scaleBuf.Get<ScaleT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        // Per-channel deqScale, here a uniform 0.5: dst = src * 0.5.
        AscendC::Duplicate<ScaleT>(deqScale, (ScaleT)0.5, ELEM);
        AscendC::PipeBarrier<PIPE_V>();

        AscendC::AscendDequant<DT, ScaleT>(zLocal, xLocal, deqScale, tmp, ELEM);

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
    AscendC::TBuf<AscendC::TPosition::VECCALC> scaleBuf;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<int32_t> xGm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelAscendDequant op;
    op.Init(x, z);
    op.Process();
}
