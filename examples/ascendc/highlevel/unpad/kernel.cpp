/*
 * Hand-written high-level unit: UnPad (adv_api/pad, the inverse of Pad).
 * UnPad removes padding columns from the right of each row. It is symmetric to Pad:
 * Pad appends rightPad elements per row, UnPad drops rightPad elements per row.
 * The 3510 UnPad impl reads tiling.{srcHeight, srcWidth} + unPadParams.rightPad only;
 * it copies (srcWidth - rightPad) valid elements per row from src to dst (leftPad is ignored on 3510).
 * Single row, srcWidth=16 (16*4=64B, 32B aligned), rightPad=4.
 * src=[1..16] -> dst[0..11]=[1..12] (the last 4 padding columns are dropped).
 */
#include "kernel_operator.h"
#include "lib/pad/pad.h"

constexpr uint32_t SRC_H = 1;
constexpr uint32_t SRC_W = 16;        // padded width, 16*4=64B -> 32B aligned
constexpr uint16_t LEFT_PAD = 0;      // ignored by the 3510 impl, kept symmetric with Pad
constexpr uint16_t RIGHT_PAD = 4;     // number of padding columns to drop on the right
constexpr uint32_t DST_W = SRC_W - RIGHT_PAD; // 12 valid elements
constexpr uint32_t TMP_BYTES = 1024;
using DT = float;

class KernelUnPad {
public:
    __aicore__ inline KernelUnPad() {}
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

        AscendC::UnPadParams params(LEFT_PAD, RIGHT_PAD);
        AscendC::tiling::UnPadTiling t;
        t.srcHeight = SRC_H;
        t.srcWidth = SRC_W;

        AscendC::UnPad<DT>(zL, xL, params, tmp, t);

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
    KernelUnPad op;
    op.Init(x, z);
    op.Process();
}
