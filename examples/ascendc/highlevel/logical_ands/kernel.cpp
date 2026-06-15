/*
 * AscendC high-level logical operator LogicalAnds (adv_api, simple count mode, no Tiling needed).
 * Tensor-scalar logical AND: dst = (src0 != 0) && (scalar != 0). Output is bool (uint8, 0/1).
 * This is the tensor-scalar counterpart of LogicalAnd (tensor-tensor, see ../logical_and/).
 * Signature on __NPU_ARCH__==3510:
 *   LogicalAnds<config, T, U, S>(const LocalTensor<T>& dst, const U& src0, const S& src1, uint32_t count)
 *   where T=bool (dst), U=LocalTensor<src dtype> (src0), S=scalar (src1).
 * Here src0=1.0 (true), scalar=1 (true) -> dst = true (1).
 */
#include "kernel_operator.h"
#include "lib/math/logical_ands.h"

constexpr uint32_t N = 64;
using ST = float;     // source dtype
using DT = bool;      // destination dtype (header requires bool, 1 byte)

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR src0, GM_ADDR dst)
    {
        src0Gm.SetGlobalBuffer((__gm__ ST *)src0, N);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, N);
        pipe.InitBuffer(inQueue0, 1, N * sizeof(ST));
        pipe.InitBuffer(outQueue, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<ST> s0 = inQueue0.AllocTensor<ST>();
        AscendC::DataCopy(s0, src0Gm, N);
        inQueue0.EnQue(s0);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<ST> s0 = inQueue0.DeQue<ST>();
        AscendC::LocalTensor<DT> d = outQueue.AllocTensor<DT>();
        AscendC::LogicalAnds(d, s0, (ST)1.0f, N);   // dst = (src0 != 0) && (1 != 0)
        outQueue.EnQue<DT>(d);
        inQueue0.FreeTensor(s0);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueue.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, N);
        outQueue.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueue0;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueue;
    AscendC::GlobalTensor<ST> src0Gm;
    AscendC::GlobalTensor<DT> dstGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src0, GM_ADDR dst)
{
    KernelOp op;
    op.Init(src0, dst);
    op.Process();
}
