/*
 * Hand-written high-level unit: Where (adv_api/math, element-wise conditional select).
 *   dst[i] = condition[i] ? src0[i] : src1[i]
 * The condition tensor has dtype bool (1 byte); src0/src1/dst share dtype float.
 * Layout for this unit: src0 = 1.0, src1 = 2.0, condition alternates true/false,
 * so dst[i] = (i even) ? 1.0 : 2.0. This exercises the real tensor/tensor branch of Where
 * and is fully verifiable on the host.
 */
#include "kernel_operator.h"
#include "lib/math/where.h"

constexpr uint32_t ELEM = 512;
using DT = float;

class KernelWhere {
public:
    __aicore__ inline KernelWhere() {}
    __aicore__ inline void Init(GM_ADDR src0, GM_ADDR src1, GM_ADDR cond, GM_ADDR dst)
    {
        src0Gm.SetGlobalBuffer((__gm__ DT *)src0, ELEM);
        src1Gm.SetGlobalBuffer((__gm__ DT *)src1, ELEM);
        condGm.SetGlobalBuffer((__gm__ bool *)cond, ELEM);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, ELEM);
        pipe.InitBuffer(inQueueSrc0, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueSrc1, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inQueueCond, 1, ELEM * sizeof(bool));
        pipe.InitBuffer(outQueueDst, 1, ELEM * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> s0 = inQueueSrc0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> s1 = inQueueSrc1.AllocTensor<DT>();
        AscendC::LocalTensor<bool> c = inQueueCond.AllocTensor<bool>();
        AscendC::DataCopy(s0, src0Gm, ELEM);
        AscendC::DataCopy(s1, src1Gm, ELEM);
        AscendC::DataCopy(c, condGm, ELEM);
        inQueueSrc0.EnQue(s0);
        inQueueSrc1.EnQue(s1);
        inQueueCond.EnQue(c);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s0 = inQueueSrc0.DeQue<DT>();
        AscendC::LocalTensor<DT> s1 = inQueueSrc1.DeQue<DT>();
        AscendC::LocalTensor<bool> c = inQueueCond.DeQue<bool>();
        AscendC::LocalTensor<DT> d = outQueueDst.AllocTensor<DT>();

        // dst = condition ? src0 : src1
        AscendC::Where(d, s0, s1, c, ELEM);

        outQueueDst.EnQue<DT>(d);
        inQueueSrc0.FreeTensor(s0);
        inQueueSrc1.FreeTensor(s1);
        inQueueCond.FreeTensor(c);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueueDst.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, ELEM);
        outQueueDst.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc0, inQueueSrc1, inQueueCond;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst;
    AscendC::GlobalTensor<DT> src0Gm, src1Gm, dstGm;
    AscendC::GlobalTensor<bool> condGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src0, GM_ADDR src1, GM_ADDR cond, GM_ADDR dst)
{
    KernelWhere op;
    op.Init(src0, src1, cond, dst);
    op.Process();
}
