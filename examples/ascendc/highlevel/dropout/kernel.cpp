/*
 * Hand-written high-level unit: DropOut (adv_api/filter, byte-mask mode, this arch=3510).
 * DropOut is NOT internally random: the keep/drop mask is supplied by the caller, so the
 * result is fully deterministic and verifiable. In byte mode the impl computes, per element:
 *   dst[i] = (1/keepProb) * (float)mask[i] * src[i]
 * (mask is a uint8 tensor whose value is cast to the compute dtype and multiplied in).
 * With dropOutMode=0 and srcLastAxis == maskLastAxis the impl selects the BYTE_ALIGN path.
 * Design [firstAxis=1, lastAxis=64], src=3.0, mask=1 (keep all), keepProb=0.5
 *   -> dst = 3.0 * 1 * (1/0.5) = 6.0 for every element. Verified on host.
 */
#include "kernel_operator.h"
#include "lib/filter/dropout.h"

constexpr uint32_t FIRST_AXIS = 1;
constexpr uint32_t LAST_AXIS = 64;                 // 64*4=256B aligned
constexpr uint32_t TOTAL = FIRST_AXIS * LAST_AXIS; // 64
constexpr uint32_t TMP_BYTES = 8192;
using DT = float;

class KernelDropOut {
public:
    __aicore__ inline KernelDropOut() {}
    __aicore__ inline void Init(GM_ADDR src, GM_ADDR mask, GM_ADDR dst)
    {
        srcGm.SetGlobalBuffer((__gm__ DT *)src, TOTAL);
        maskGm.SetGlobalBuffer((__gm__ uint8_t *)mask, TOTAL);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, TOTAL);
        pipe.InitBuffer(inQueueSrc, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(inQueueMask, 1, TOTAL * sizeof(uint8_t) + 32);
        pipe.InitBuffer(outQueueDst, 1, TOTAL * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> sL = inQueueSrc.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> mL = inQueueMask.AllocTensor<uint8_t>();
        AscendC::DataCopy(sL, srcGm, TOTAL);
        AscendC::DataCopy(mL, maskGm, TOTAL);
        inQueueSrc.EnQue(sL);
        inQueueMask.EnQue(mL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> sL = inQueueSrc.DeQue<DT>();
        AscendC::LocalTensor<uint8_t> mL = inQueueMask.DeQue<uint8_t>();
        AscendC::LocalTensor<DT> dL = outQueueDst.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::DropOutShapeInfo info;
        info.firstAxis = FIRST_AXIS;
        info.srcLastAxis = LAST_AXIS;
        info.maskLastAxis = LAST_AXIS;            // equal -> byte-align path

        const float keepProb = 0.5f;
        // T, isInitBitMode=false, dropOutMode=0 (auto-select byte/bit by the axis relation)
        AscendC::DropOut<DT>(dL, sL, mL, tmp, keepProb, info);

        outQueueDst.EnQue<DT>(dL);
        inQueueSrc.FreeTensor(sL);
        inQueueMask.FreeTensor(mL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> dL = outQueueDst.DeQue<DT>();
        AscendC::DataCopy(dstGm, dL, TOTAL);
        outQueueDst.FreeTensor(dL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc, inQueueMask;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> srcGm, dstGm;
    AscendC::GlobalTensor<uint8_t> maskGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src, GM_ADDR mask, GM_ADDR dst)
{
    KernelDropOut op;
    op.Init(src, mask, dst);
    op.Process();
}
