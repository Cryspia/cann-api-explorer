/*
 * Hand-written high-level unit: CumSum (adv_api/math, prefix sum along the last axis).
 *   For a [outter, inner] tensor, dst[r][c] = sum(src[r][0..c]).
 * The default CumSumConfig is {isLastAxis=true, isReuseSource=false, outputLastRow=true},
 * so CumSum also writes the prefix sum of the final row into lastRowTensor.
 *
 * Shape [16,16]: outter and inner are both multiples of NCHW_CONV_ADDR_LIST_SIZE(16),
 * which the device-side transpose path needs. inner=16 floats = 64 bytes (32-byte aligned).
 * Input is all ones, so each output row becomes [1,2,3,...,16] and lastRow = [1,2,...,16].
 * dst layout in GM: [16 rows][16 cols] followed by lastRow [16] -> we pack both into one
 * output buffer so the host can verify them together.
 */
#include "kernel_operator.h"
#include "lib/math/cumsum.h"

constexpr uint32_t OUTTER = 16;
constexpr uint32_t INNER = 16;
constexpr uint32_t ELEM = OUTTER * INNER;   // 256
constexpr uint32_t TMP_BYTES = 16384;       // ample temporary space (min = 16*INNER*4*2 = 2048)
using DT = float;

class KernelCumSum {
public:
    __aicore__ inline KernelCumSum() {}
    __aicore__ inline void Init(GM_ADDR src, GM_ADDR dst, GM_ADDR lastRow)
    {
        srcGm.SetGlobalBuffer((__gm__ DT *)src, ELEM);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, ELEM);
        lastRowGm.SetGlobalBuffer((__gm__ DT *)lastRow, INNER);
        pipe.InitBuffer(inQueueSrc, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueDst, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueLast, 1, INNER * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.AllocTensor<DT>();
        AscendC::DataCopy(s, srcGm, ELEM);
        inQueueSrc.EnQue(s);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.DeQue<DT>();
        AscendC::LocalTensor<DT> d = outQueueDst.AllocTensor<DT>();
        AscendC::LocalTensor<DT> last = outQueueLast.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::CumSumInfo info{OUTTER, INNER};
        // Default config: last-axis prefix sum, also emits the prefix sum of the final row.
        AscendC::CumSum<DT>(d, last, s, tmp, info);

        outQueueDst.EnQue<DT>(d);
        outQueueLast.EnQue<DT>(last);
        inQueueSrc.FreeTensor(s);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueueDst.DeQue<DT>();
        AscendC::LocalTensor<DT> last = outQueueLast.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, ELEM);
        AscendC::DataCopy(lastRowGm, last, INNER);
        outQueueDst.FreeTensor(d);
        outQueueLast.FreeTensor(last);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst, outQueueLast;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> srcGm, dstGm, lastRowGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src, GM_ADDR dst, GM_ADDR lastRow)
{
    KernelCumSum op;
    op.Init(src, dst, lastRow);
    op.Process();
}
