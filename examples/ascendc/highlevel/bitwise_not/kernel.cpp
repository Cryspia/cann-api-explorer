/*
 * AscendC high-level math operator BitwiseNot (adv_api, simple count mode, no Tiling needed).
 * Elementwise bitwise NOT (one's complement) of an integer tensor: dst[i] = ~src[i].
 * Relation: this is the adv_api (lib/math) count-mode entry; the same per-element bitwise NOT
 * is also exposed by the lower-level vector intrinsic AscendC::Not (see ../../vector/not/).
 * Signature on __NPU_ARCH__==3510:
 *   BitwiseNot<config, T>(const LocalTensor<T>& dst, const LocalTensor<T>& src, uint32_t count)
 * Level-2 integer dtypes on this SoC: int16/uint16/int64/uint64 (int32 is NOT supported),
 * so int16_t is used. Two's complement: ~5 = -6 (signed int16).
 */
#include "kernel_operator.h"
#include "lib/math/bitwise_not.h"

constexpr uint32_t N = 64;
using DT = int16_t;

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::BitwiseNot(zL, xL, N);   // dst = ~src
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelOp op;
    op.Init(x, z);
    op.Process();
}
