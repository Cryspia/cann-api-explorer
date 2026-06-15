/*
 * Hand-written high-level unit: AscendAntiQuant (adv_api quantization).
 * Formula (per header / impl): dst = scale * (src + offset).
 * Per-tensor scalar overload: offset and scale are scalars of the output type (half).
 * src is int8, dst is half. Non-transpose path processes all src.GetSize() elements.
 */
#include "kernel_operator.h"
#include "lib/quantization/ascend_antiquant.h"

constexpr uint32_t ELEM = 256;                 // element count
constexpr uint32_t TMP_BYTES = 8192;           // ample shared temporary space
using ST = int8_t;                             // quantized source type
using DT = half;                               // dequantized output type

class KernelAscendAntiQuant {
public:
    __aicore__ inline KernelAscendAntiQuant() {}
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

        // Scalar offset / scale: dst = scale * (src + offset) = 1.0 * (src + 0.0).
        const DT offset = (DT)0.0;
        const DT scale = (DT)1.0;
        // isTranspose=false; K is the inner length (used only by the transpose path).
        AscendC::AscendAntiQuant<ST, DT, false>(zLocal, xLocal, offset, scale, tmp, ELEM);

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
    KernelAscendAntiQuant op;
    op.Init(x, z);
    op.Process();
}
