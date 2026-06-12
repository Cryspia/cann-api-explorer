/*
 * Auto-generated: AscendC reduction operator ReduceMin (reduces ELEM elements to a scalar, written to dst[0]).
 * Single core, single tile, with a VECCALC scratch space sharedTmpBuffer.
 * Rendered by harness/gen.py from manifest.yaml; do not edit by hand.
 */
#include "kernel_operator.h"

constexpr int32_t ELEM = 256;          // number of elements taking part in the reduction
using DT = float;

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inQueueX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(calcBuf, ELEM * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm, ELEM);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<DT> tmp = calcBuf.Get<DT>();
        AscendC::ReduceMin(zLocal, xLocal, tmp, ELEM);
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
    AscendC::TBuf<AscendC::TPosition::VECCALC> calcBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelOp op;
    op.Init(x, z);
    op.Process();
}
