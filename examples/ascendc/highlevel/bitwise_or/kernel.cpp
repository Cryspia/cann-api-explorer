/*
 * AscendC high-level math operator BitwiseOr (adv_api, simple count mode, no Tiling needed).
 * Elementwise bitwise OR of two integer tensors: dst[i] = src0[i] | src1[i].
 * Relation: this is the adv_api (lib/math) count-mode entry; the same per-element bitwise OR
 * is also exposed by the lower-level vector intrinsic AscendC::Or (see ../../vector/or/).
 * Signature on __NPU_ARCH__==3510:
 *   BitwiseOr<config, T>(const LocalTensor<T>& dst, const LocalTensor<T>& src0,
 *                        const LocalTensor<T>& src1, uint32_t count)
 * Level-2 integer dtypes on this SoC: int16/uint16/int64/uint64 (int32 is NOT supported),
 * so int16_t is used. Example: 0b1100 (12) | 0b1010 (10) = 0b1110 (14).
 */
#include "kernel_operator.h"
#include "lib/math/bitwise_or.h"

constexpr uint32_t N = 64;
using DT = int16_t;

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        yGm.SetGlobalBuffer((__gm__ DT *)y, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inY, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> yL = inY.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(yL, yGm, N);
        inX.EnQue(xL);
        inY.EnQue(yL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> yL = inY.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::BitwiseOr(zL, xL, yL, N);   // dst = src0 | src1
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
        inY.FreeTensor(yL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inY;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, yGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z)
{
    KernelOp op;
    op.Init(x, y, z);
    op.Process();
}
