/*
 * AscendC granularity-reduction operator RepeatReduceSum.
 * 3510-specific: reduce mask elements of each repeat to 1 (extra dstBlkStride param vs WholeReduceSum).
 * Single core, single tile. The result LocalTensor is copied back to GM in
 * whole 32-byte blocks (DataCopy on GM requires 32B-aligned element counts).
 */
#include "kernel_operator.h"

constexpr int32_t SRC_ELEM = 512;      // number of input elements
constexpr int32_t DST_ELEM = 8;   // 32B-aligned output element count copied to GM
constexpr int32_t REPEAT   = 8;   // repeat times
constexpr int32_t MASK     = 64;     // elements processed per repeat
using DT = float;

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, SRC_ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, DST_ELEM);
        pipe.InitBuffer(inQueueX, 1, SRC_ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, DST_ELEM * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm, SRC_ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        // dstRepStride / srcBlkStride / srcRepStride in 32B-block units.
        AscendC::RepeatReduceSum<DT>(zLocal, xLocal, REPEAT, MASK, 1, 1, 1, 8);
        outQueueZ.EnQue<DT>(zLocal);
        inQueueX.FreeTensor(xLocal);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zLocal, DST_ELEM);
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelOp op;
    op.Init(x, z);
    op.Process();
}
