/*
 * Hand-written unit: chain CompareScalar + Select in a single kernel (the two flagship cmpsel APIs).
 *   mask = CompareScalar(x > 0)  -> all true (x=1.0)
 *   z    = Select(mask, x, y)    -> take x -> z = x = 1.0
 * This avoids bit-by-bit verification of the mask; just verify the float output of Select.
 */
#include "kernel_operator.h"

constexpr int32_t TOTAL_LENGTH = 8 * 2048;
constexpr int32_t USE_CORE_NUM = 8;
constexpr int32_t BLOCK_LENGTH = TOTAL_LENGTH / USE_CORE_NUM;
constexpr int32_t TILE_NUM = 8;
constexpr int32_t BUFFER_NUM = 2;
constexpr int32_t TILE_LENGTH = BLOCK_LENGTH / TILE_NUM / BUFFER_NUM;
using DT = float;

class KernelSelect {
public:
    __aicore__ inline KernelSelect() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        yGm.SetGlobalBuffer((__gm__ DT *)y + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        zGm.SetGlobalBuffer((__gm__ DT *)z + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
        pipe.InitBuffer(inQueueY, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
        pipe.InitBuffer(outQueueZ, BUFFER_NUM, TILE_LENGTH * sizeof(DT));
        pipe.InitBuffer(maskBuf, TILE_LENGTH * sizeof(uint8_t));
    }
    __aicore__ inline void Process()
    {
        int32_t loopCount = TILE_NUM * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) { CopyIn(i); Compute(i); CopyOut(i); }
    }
private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.AllocTensor<DT>();
        AscendC::LocalTensor<DT> yLocal = inQueueY.AllocTensor<DT>();
        AscendC::DataCopy(xLocal, xGm[progress * TILE_LENGTH], TILE_LENGTH);
        AscendC::DataCopy(yLocal, yGm[progress * TILE_LENGTH], TILE_LENGTH);
        inQueueX.EnQue(xLocal);
        inQueueY.EnQue(yLocal);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        AscendC::LocalTensor<DT> xLocal = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> yLocal = inQueueY.DeQue<DT>();
        AscendC::LocalTensor<DT> zLocal = outQueueZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> mask = maskBuf.Get<uint8_t>();
        // mask = (x > 0) -> all true
        AscendC::CompareScalar(mask, xLocal, (DT)0.0, AscendC::CMPMODE::GT, TILE_LENGTH);
        // z = mask ? x : y -> take x
        AscendC::Select(zLocal, mask, xLocal, yLocal,
                        AscendC::SELMODE::VSEL_TENSOR_TENSOR_MODE, TILE_LENGTH);
        outQueueZ.EnQue<DT>(zLocal);
        inQueueX.FreeTensor(xLocal);
        inQueueY.FreeTensor(yLocal);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        AscendC::LocalTensor<DT> zLocal = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm[progress * TILE_LENGTH], zLocal, TILE_LENGTH);
        outQueueZ.FreeTensor(zLocal);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> inQueueX, inQueueY;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> outQueueZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> maskBuf;
    AscendC::GlobalTensor<DT> xGm, yGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z)
{
    KernelSelect op;
    op.Init(x, y, z);
    op.Process();
}
