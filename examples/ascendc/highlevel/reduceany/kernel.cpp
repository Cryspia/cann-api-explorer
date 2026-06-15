/*
 * Hand-written high-level unit: ReduceAny (adv_api reduce, last-axis reduction).
 * ReduceAny<T, Pattern::Reduce::AR>(dst, src, sharedTmpBuffer, srcShape, srcInnerPad):
 *   logical "any element true" reduction along the last (R) axis. For uint8_t input
 *   the result is the max element, so any nonzero -> nonzero, all-zero -> 0.
 * Layout [dimA=1, dimR=64] (64 bytes, 32-byte aligned). Result lands in dst[0],
 *   copied out as a full 32-byte aligned block (DataCopy minimum transfer).
 */
#include "kernel_operator.h"
#include "adv_api/reduce/reduce.h"

constexpr uint32_t DIM_A = 1;
constexpr uint32_t DIM_R = 64;             // DIM_R*sizeof(uint8_t)=64, 32-byte aligned
constexpr uint32_t ELEM = DIM_A * DIM_R;   // 64
constexpr uint32_t TMP_BYTES = 16384;
using DT = uint8_t;

class KernelReduceAny {
public:
    __aicore__ inline KernelReduceAny() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, ELEM);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        uint32_t srcShape[2] = {DIM_A, DIM_R};
        AscendC::ReduceAny<DT, AscendC::Pattern::Reduce::AR>(zL, xL, tmp, srcShape, false);

        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        // Result lands in zL[0]; copy the whole ELEM-sized (32-byte aligned) buffer.
        AscendC::DataCopy(zGm, zL, ELEM);
        outZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelReduceAny op;
    op.Init(x, z);
    op.Process();
}
