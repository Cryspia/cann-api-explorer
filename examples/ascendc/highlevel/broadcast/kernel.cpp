/*
 * Hand-written high-level unit: Broadcast (adv_api/pad/broadcast, 3510 computes tiling device-side).
 * Broadcast src [1, N] along axis0 to dst [M, N]: replicate src for each row.
 * 3510 path: GetBroadcastTilingInfo inside the kernel computes BroadcastTiling -> Broadcast(..., &tiling).
 * tmp is obtained internally via PopStackBuffer from the remaining UB of the TPipe (small data volume, UB is sufficient).
 * src=[0..7] -> dst[i][j]=j (every row is 0..7).
 */
#include "kernel_operator.h"
#include "lib/pad/broadcast.h"

constexpr uint32_t M = 4;   // number of dst rows
constexpr uint32_t N = 8;   // last dimension (8*4=32B aligned)
using DT = float;

class KernelBroadcast {
public:
    __aicore__ inline KernelBroadcast() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, M * N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(outZ, 1, M * N * sizeof(DT));
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

        uint32_t srcShape[2] = {1, N};
        uint32_t dstShape[2] = {M, N};
        AscendC::BroadcastTiling tiling;
        AscendC::GetBroadcastTilingInfo<DT>(2, dstShape, srcShape, /*srcInnerPad*/ false, tiling);
        AscendC::Broadcast<DT>(zL, xL, dstShape, srcShape, &tiling);

        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, M * N);
        outZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelBroadcast op;
    op.Init(x, z);
    op.Process();
}
