/*
 * Hand-written vector unit: Gatherb (block-level gather, distinct from element-wise Gather).
 * Gatherb(dst, src0, offset, repeatTime, GatherRepeatParams): for each of the offsets it fetches
 * one 32-byte block (= 8 float elements) from src0 at the given BYTE offset, and writes the
 * blocks contiguously into dst. One repeat consumes DEFAULT_BLK_NUM (8) offsets -> 8 blocks.
 * Here N=64 floats = 8 blocks of 8 floats. offset[k] = (NUM_BLK-1-k)*32 bytes selects the blocks
 * in reverse order, so dst is src with its 8 blocks reversed (block granularity, not element).
 */
#include "kernel_operator.h"

constexpr uint32_t N = 64;              // 64 floats = 8 blocks of 8 floats (32 bytes each)
constexpr uint32_t NUM_BLK = 8;        // number of 32-byte blocks gathered (one repeat)
using DT = float;

class KernelGatherb {
public:
    __aicore__ inline KernelGatherb() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR off, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        offGm.SetGlobalBuffer((__gm__ uint32_t *)off, NUM_BLK);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inOff, 1, NUM_BLK * sizeof(uint32_t));
        pipe.InitBuffer(outZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<uint32_t> oL = inOff.AllocTensor<uint32_t>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(oL, offGm, NUM_BLK);
        inX.EnQue(xL); inOff.EnQue(oL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<uint32_t> oL = inOff.DeQue<uint32_t>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        // One repeat gathers NUM_BLK (8) blocks; default GatherRepeatParams writes them contiguously.
        AscendC::GatherRepeatParams params;
        AscendC::Gatherb<DT>(zL, xL, oL, (uint8_t)1, params);
        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL); inOff.FreeTensor(oL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outZ.FreeTensor(zL);
    }
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inOff;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
    AscendC::GlobalTensor<uint32_t> offGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR off, GM_ADDR z)
{
    KernelGatherb op;
    op.Init(x, off, z);
    op.Process();
}
