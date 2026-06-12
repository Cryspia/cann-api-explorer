/*
 * Hand-written high-level unit: Pad (adv_api/pad, 3510 only needs 3 hand-filled tiling fields).
 * Pads each row on the right with rightPad copies of padValue. Picks the simplest "aligned" path: single row, srcWidth aligned to 32B and < one reg block.
 * 3510 PadImpl only reads tiling.{srcHeight, srcWidth, srcOriWidth} + padParams.{leftPad,rightPad,padValue}.
 * srcWidth=16(<regBlockElementCnt=CUBE_MAX_SIZE/4=64), srcOriWidth=12, rightPad=4, padValue=7.
 * src=[1..16] -> dst[0..11]=src[0..11], dst[12..15]=7.
 */
#include "kernel_operator.h"
#include "lib/pad/pad.h"

constexpr uint32_t SRC_H = 1;
constexpr uint32_t SRC_W = 16;        // 16*4=64B -> 32B aligned (takes AlignedPad), and < 64=regBlockElementCnt
constexpr uint32_t SRC_ORI_W = 12;    // valid width
constexpr uint16_t LEFT_PAD = 0;      // the aligned path does not handle leftPad, set to 0
constexpr uint16_t RIGHT_PAD = 4;     // SRC_ORI_W + RIGHT_PAD = 16 = SRC_W -> the tail is filled exactly
constexpr int32_t PAD_VALUE = 7;
constexpr uint32_t TMP_BYTES = 1024;
using DT = float;

class KernelPad {
public:
    __aicore__ inline KernelPad() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, SRC_W);
        zGm.SetGlobalBuffer((__gm__ DT *)z, SRC_W);
        pipe.InitBuffer(inX, 1, SRC_W * sizeof(DT));
        pipe.InitBuffer(outZ, 1, SRC_W * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, SRC_W);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::PadParams params(LEFT_PAD, RIGHT_PAD, PAD_VALUE);
        AscendC::tiling::PadTiling t;
        t.srcHeight = SRC_H;
        t.srcWidth = SRC_W;
        t.srcOriWidth = SRC_ORI_W;

        AscendC::Pad<DT>(zL, xL, params, tmp, t);

        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, SRC_W);
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
    KernelPad op;
    op.Init(x, z);
    op.Process();
}
