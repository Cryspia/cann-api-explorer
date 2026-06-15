/*
 * Hand-written high-level unit: Quantize (adv_api/quantization, the general Quantize API).
 * Distinct from AscendQuant: Quantize takes a QuantizeConfig (policy/hasOffset/roundMode/kDim)
 * non-type template parameter and a QuantizeParams{m,n,groupSize} struct, and supports
 * per-tensor / per-channel / per-token / per-group policies with a generic dst type.
 * Formula (per-tensor): dst = round(src * scale + offset).
 *
 * This example uses the simplest PER_TENSOR policy with scalar scale/offset (no tmp buffer).
 * src=2.0 (float), scale=2.0, offset=1.0 -> dst = round(2*2 + 1) = 5 (int8).
 */
#include "kernel_operator.h"
#include "lib/quantization/quantize.h"

constexpr uint32_t ROWS = 1;
constexpr uint32_t COLS = 256;
constexpr uint32_t ELEM = ROWS * COLS;
using ST = float;                              // source type
using DT = int8_t;                             // quantized output type

class KernelQuantize {
public:
    __aicore__ inline KernelQuantize() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ ST *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inX, 1, ELEM * sizeof(ST));
        pipe.InitBuffer(outZ, 1, ELEM * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> xL = inX.AllocTensor<ST>();
        AscendC::DataCopy(xL, xGm, ELEM);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> xL = inX.DeQue<ST>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();

        // QuantizeConfig / QuantizeParams live behind the __NPU_ARCH__==3510 guard in
        // quantize.h, so they are only visible in the device compile pass. The host parse
        // pass (--cce-host-only, no __NPU_ARCH__) skips this block.
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3510 || __NPU_ARCH__ == 5102)
        // PER_TENSOR with offset; CAST_RINT rounding (round to nearest).
        static constexpr AscendC::QuantizeConfig QCFG = {
            AscendC::QuantizePolicy::PER_TENSOR, true, AscendC::RoundMode::CAST_RINT, 1};

        const float scale = 2.0f;
        const float offset = 1.0f;
        AscendC::QuantizeParams params;
        params.m = ROWS;
        params.n = COLS;
        params.groupSize = 0;

        AscendC::Quantize<QCFG, DT, ST, float, float>(zL, xL, scale, offset, params);
#endif

        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, ELEM);
        outZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<ST> xGm;
    AscendC::GlobalTensor<DT> zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelQuantize op;
    op.Init(x, z);
    op.Process();
}
