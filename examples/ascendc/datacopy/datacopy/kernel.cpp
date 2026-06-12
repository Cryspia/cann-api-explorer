/*
 * Hand-written sample: DataCopy (data-movement API).
 * Pure-movement identity kernel: GM(x) --DataCopy--> Local --DataCopy--> GM(z), z == x.
 * Specifically highlights AscendC::DataCopy(GM<->Local) itself, performing no computation.
 */
#include "kernel_operator.h"

constexpr int32_t TOTAL_LENGTH = 8 * 2048;
constexpr int32_t USE_CORE_NUM = 8;
constexpr int32_t BLOCK_LENGTH = TOTAL_LENGTH / USE_CORE_NUM;
constexpr int32_t TILE_NUM = 8;
constexpr int32_t BUFFER_NUM = 2;
constexpr int32_t TILE_LENGTH = BLOCK_LENGTH / TILE_NUM / BUFFER_NUM;
using DT = float;

class KernelCopy {
public:
    __aicore__ inline KernelCopy() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        zGm.SetGlobalBuffer((__gm__ DT *)z + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
        pipe.InitBuffer(outQueueZ, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
    }
    __aicore__ inline void Process()
    {
        int32_t loopCount = TILE_NUM * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) { CopyIn(i); CopyOut(i); }
    }
private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm[progress * TILE_LENGTH], TILE_LENGTH);   // GM -> Local
        inQueueX.EnQue(xLocal);
        // Directly move out the data just read in (identity)
        AscendC::LocalTensor<DT> xIn = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::DataCopy(zLocal, xIn, TILE_LENGTH);                           // Local -> Local
        outQueueZ.EnQue<DT>(zLocal);
        inQueueX.FreeTensor(xIn);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm[progress * TILE_LENGTH], zLocal, TILE_LENGTH);   // Local -> GM
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> outQueueZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelCopy op;
    op.Init(x, z);
    op.Process();
}
