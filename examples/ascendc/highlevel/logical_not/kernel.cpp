/*
 * AscendC high-level logical operator LogicalNot (adv_api, simple count mode, no Tiling needed).
 * Elementwise logical NOT: dst = !(src != 0). Output is bool (0/1).
 */
#include "kernel_operator.h"
#include "lib/math/logical_not.h"

constexpr int32_t TOTAL_LENGTH = 8 * 2048;
constexpr int32_t USE_CORE_NUM = 8;
constexpr int32_t BLOCK_LENGTH = TOTAL_LENGTH / USE_CORE_NUM;
constexpr int32_t TILE_NUM = 8;
constexpr int32_t BUFFER_NUM = 2;
constexpr int32_t TILE_LENGTH = BLOCK_LENGTH / TILE_NUM / BUFFER_NUM;
using ST = float;   // source dtype
using DT = bool;    // destination dtype (header requires bool, 1 byte)

class KernelOp {
public:
    __aicore__ inline KernelOp() {}
    __aicore__ inline void Init(GM_ADDR src, GM_ADDR dst)
    {
        srcGm.SetGlobalBuffer((__gm__ ST *)src + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        pipe.InitBuffer(inQueue, BUFFER_NUM, TILE_LENGTH * sizeof(ST));
        pipe.InitBuffer(outQueue, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
    }
    __aicore__ inline void Process()
    {
        int32_t loopCount = TILE_NUM * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) { CopyIn(i); Compute(i); CopyOut(i); }
    }
private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        AscendC::LocalTensor<ST> s = inQueue.AllocTensor<ST>();
        AscendC::DataCopy(s, srcGm[progress * TILE_LENGTH], TILE_LENGTH);
        inQueue.EnQue(s);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        AscendC::LocalTensor<ST> s = inQueue.DeQue<ST>();
        AscendC::LocalTensor<DT> d = outQueue.AllocTensor<DT>();
        AscendC::LogicalNot(d, s, TILE_LENGTH);
        outQueue.EnQue<DT>(d);
        inQueue.FreeTensor(s);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        AscendC::LocalTensor<DT> d = outQueue.DeQue<DT>();
        AscendC::DataCopy(dstGm[progress * TILE_LENGTH], d, TILE_LENGTH);
        outQueue.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> inQueue;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> outQueue;
    AscendC::GlobalTensor<ST> srcGm;
    AscendC::GlobalTensor<DT> dstGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src, GM_ADDR dst)
{
    KernelOp op;
    op.Init(src, dst);
    op.Process();
}
