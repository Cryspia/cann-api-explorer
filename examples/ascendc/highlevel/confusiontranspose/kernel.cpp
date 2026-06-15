/*
 * High-level unit: ConfusionTranspose (adv_api/transpose/confusion_transpose.h).
 *
 * Public API (confirmed for __NPU_ARCH__==3510):
 *   AscendC::Transpose<T>(dst, src, sharedTmpBuffer, TransposeType, ConfusionTransposeTiling&);
 *
 * This is the "confusion transpose" family used for attention reshape+transpose. The simplest
 * verifiable mode available on 3510 is TRANSPOSE_ND2ND_021: it transposes the last two axes of a
 * [dim0, dim1, dim2] ND tensor -> [dim0, dim2, dim1]. With dim0=1 it degenerates to a plain
 * 2D transpose [H,W] -> [W,H].
 *
 * The tiling for this mode is a 3-field struct {dim0, dim1, dim2} that the impl obtains by
 * reinterpret_cast-ing ConfusionTransposeTiling. ConfusionTransposeTiling exposes 18 generic
 * uint32 params (param0..param17); the first three map directly to dim0/dim1/dim2.
 *
 * Design: H=W=16, T=half. src[h][w] = h*16 + w. Expect dst[w][h] = h*16 + w, i.e.
 * dst[i][j] = j*16 + i. Host verifies the full 16x16 transposed matrix.
 */
#include "kernel_operator.h"
#include "adv_api/transpose/confusion_transpose.h"

constexpr uint32_t H = 16;
constexpr uint32_t W = 16;
constexpr uint32_t ELEM = H * W;       // 256
constexpr uint32_t TMP_BYTES = 8192;
using DT = half;

class KernelConfusionTranspose {
public:
    __aicore__ inline KernelConfusionTranspose() {}
    __aicore__ inline void Init(GM_ADDR src, GM_ADDR dst)
    {
        srcGm.SetGlobalBuffer((__gm__ DT *)src, ELEM);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, ELEM);
        pipe.InitBuffer(inQueueSrc, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueDst, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.AllocTensor<DT>();
        AscendC::DataCopy(s, srcGm, ELEM);
        inQueueSrc.EnQue(s);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.DeQue<DT>();
        AscendC::LocalTensor<DT> d = outQueueDst.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        // ConfusionTransposeTiling: param0=dim0, param1=dim1, param2=dim2 (reinterpret-cast inside the impl).
        AscendC::tiling::ConfusionTransposeTiling tiling;
        tiling.param0 = 1;     // dim0
        tiling.param1 = H;     // dim1
        tiling.param2 = W;     // dim2
        AscendC::Transpose<DT>(d, s, tmp, AscendC::TransposeType::TRANSPOSE_ND2ND_021, tiling);

        outQueueDst.EnQue<DT>(d);
        inQueueSrc.FreeTensor(s);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueueDst.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, ELEM);
        outQueueDst.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> srcGm, dstGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src, GM_ADDR dst)
{
    KernelConfusionTranspose op;
    op.Init(src, dst);
    op.Process();
}
