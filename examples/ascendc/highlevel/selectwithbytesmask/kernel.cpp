/*
 * High-level unit: SelectWithBytesMask (adv_api/select).
 *
 * Public API (tensor-scalar overload, confirmed for __NPU_ARCH__==3510):
 *   AscendC::Select<T,U,isReuseMask>(dst, src0Tensor, src1Scalar, maskTensor,
 *                                    sharedTmpBuffer, SelectWithBytesMaskShapeInfo);
 *
 * Semantics (per header doc and 3510 impl):
 *   mask element == 0  -> take src0 tensor value
 *   mask element != 0  -> take src1 scalar value
 *
 * Shape is viewed as [firstAxis, srcLastAxis]; the mask has its own [firstAxis, maskLastAxis]
 * (maskLastAxis >= srcLastAxis, 32B aligned, multiple of 16). Here src and mask share the same
 * last axis so the mapping is element-to-element.
 *
 * Design: shape [16,16], T=half. src0[i] = i, src1 scalar = 99.0. The mask is built so that
 * each row's first 8 columns are 0 (-> keep src0=index) and the last 8 columns are 1 (-> 99.0),
 * giving a result that the host can verify position by position.
 */
#include "kernel_operator.h"
#include "adv_api/select/selectwithbytesmask.h"

constexpr uint32_t FIRST_AXIS = 16;
constexpr uint32_t LAST_AXIS = 16;            // 16 half = 32B aligned, multiple of 16
constexpr uint32_t ELEM = FIRST_AXIS * LAST_AXIS;   // 256
constexpr half SCALAR_VAL = (half)99.0;
constexpr uint32_t TMP_BYTES = 8192;
using DT = half;
using MT = uint8_t;

class KernelSelectBytesMask {
public:
    __aicore__ inline KernelSelectBytesMask() {}
    __aicore__ inline void Init(GM_ADDR src0, GM_ADDR mask, GM_ADDR dst)
    {
        src0Gm.SetGlobalBuffer((__gm__ DT *)src0, ELEM);
        maskGm.SetGlobalBuffer((__gm__ MT *)mask, ELEM);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, ELEM);
        pipe.InitBuffer(inQueueSrc, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueMask, 1, ELEM * sizeof(MT));
        pipe.InitBuffer(outQueueDst, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> s0 = inQueueSrc.AllocTensor<DT>();
        AscendC::LocalTensor<MT> m = inQueueMask.AllocTensor<MT>();
        AscendC::DataCopy(s0, src0Gm, ELEM);
        AscendC::DataCopy(m, maskGm, ELEM);
        inQueueSrc.EnQue(s0);
        inQueueMask.EnQue(m);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s0 = inQueueSrc.DeQue<DT>();
        AscendC::LocalTensor<MT> m = inQueueMask.DeQue<MT>();
        AscendC::LocalTensor<DT> d = outQueueDst.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::SelectWithBytesMaskShapeInfo info;
        info.firstAxis = FIRST_AXIS;
        info.srcLastAxis = LAST_AXIS;
        info.maskLastAxis = LAST_AXIS;
        // mask==0 -> src0 tensor ; mask!=0 -> src1 scalar
        AscendC::Select<DT, MT, true>(d, s0, SCALAR_VAL, m, tmp, info);

        outQueueDst.EnQue<DT>(d);
        inQueueSrc.FreeTensor(s0);
        inQueueMask.FreeTensor(m);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueueDst.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, ELEM);
        outQueueDst.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc, inQueueMask;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> src0Gm, dstGm;
    AscendC::GlobalTensor<MT> maskGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src0, GM_ADDR mask, GM_ADDR dst)
{
    KernelSelectBytesMask op;
    op.Init(src0, mask, dst);
    op.Process();
}
