/*
 * Hand-written high-level unit: AscendQuant (adv_api quantization, per-tensor).
 * Formula: dst(int8) = round(src(float) * scale + offset).
 * Per-tensor overload: scale and offset are scalar floats.
 * Shape: a single block of ELEM float elements, quantized to int8.
 */
#include "kernel_operator.h"
#include "lib/quantization/ascend_quant.h"

constexpr uint32_t ELEM = 256;                 // element count
constexpr uint32_t TMP_BYTES = 8192;           // ample shared temporary space
using ST = float;                              // source type (half/float supported)
using DT = int8_t;                             // quantized output type

class KernelAscendQuant {
public:
    __aicore__ inline KernelAscendQuant() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ ST *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(ST));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> xLocal = inQueueX.AllocTensor<ST>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> xLocal = inQueueX.DeQue<ST>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        // Per-tensor quant: scalar scale / offset. scale=1.0, offset=0.0.
        const float scale = 1.0f;
        const float offset = 0.0f;
        AscendC::AscendQuant<ST>(zLocal, xLocal, tmp, scale, offset, ELEM);

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
    AscendC::GlobalTensor<ST> xGm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelAscendQuant op;
    op.Init(x, z);
    op.Process();
}
