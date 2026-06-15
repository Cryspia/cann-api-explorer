/*
 * High-level unit: TransData (adv_api/transpose/transdata.h).
 *
 * Public API (confirmed for __NPU_ARCH__==3510):
 *   template <const TransDataConfig& config, typename T, typename U, typename S>
 *   AscendC::TransData(dst, src, sharedTmpBuffer, TransDataParams<U,S>{srcLayout, dstLayout});
 *
 * On 3510 TransData performs 5D format conversion between NCDHW and the fractal layouts
 * NDC1HWC0 / FRACTAL_Z_3D (data type half/bfloat16/uint16/int16). The src/dst layouts are
 * CuTe-style Layout objects; the impl only reads the NCDHW-side shape (n,c,d,h,w) to drive the
 * TransDataTo5HD reorder.
 *
 * Rather than hand-decoding the exact fractal byte layout, this unit verifies a *round trip*:
 *   NCDHW -> NDC1HWC0 -> NCDHW
 * which must reproduce the original tensor for the non-padded region. Shape NCDHW = [1,16,1,1,1]
 * (C=16=c0, so no channel padding; H=W=1). The 16 channel values must come back unchanged.
 */
#include "kernel_operator.h"
#include "adv_api/transpose/transdata.h"

constexpr int32_t N = 1;
constexpr int32_t C = 16;
constexpr int32_t D = 1;
constexpr int32_t H = 4;
constexpr int32_t W = 4;        // H*W = 16 -> 32B aligned, no spatial padding
constexpr int32_t C0 = 16;
constexpr int32_t C1 = (C + C0 - 1) / C0;          // 1
constexpr int32_t NCDHW_ELEM = N * C * D * H * W;   // 16
// NDC1HWC0 element count: N*D*C1*H*W*C0
constexpr int32_t HWC0 = H * W * C0;
constexpr int32_t SIXHD_ELEM = N * D * C1 * H * W * C0;  // 16
constexpr int32_t MID_BYTES = SIXHD_ELEM * (int32_t)sizeof(half) > 64
                                  ? SIXHD_ELEM * (int32_t)sizeof(half) : 64;
constexpr uint32_t TMP_BYTES = 16384;
using DT = half;

// TransData and its TransDataConfig type only exist for device architectures (here 3510).
// On the host-only compile pass __NPU_ARCH__ is undefined, so guard the device code.
#if defined(__NPU_ARCH__)
// Forward config: NCDHW -> NDC1HWC0
constexpr AscendC::TransDataConfig CFG_FWD{AscendC::DataFormat::NCDHW, AscendC::DataFormat::NDC1HWC0};
// Backward config: NDC1HWC0 -> NCDHW
constexpr AscendC::TransDataConfig CFG_BWD{AscendC::DataFormat::NDC1HWC0, AscendC::DataFormat::NCDHW};
#endif

class KernelTransData {
public:
    __aicore__ inline KernelTransData() {}
    __aicore__ inline void Init(GM_ADDR src, GM_ADDR dst)
    {
        srcGm.SetGlobalBuffer((__gm__ DT *)src, NCDHW_ELEM);
        dstGm.SetGlobalBuffer((__gm__ DT *)dst, NCDHW_ELEM);
        pipe.InitBuffer(inQueueSrc, 1, NCDHW_ELEM * sizeof(DT));
        pipe.InitBuffer(outQueueDst, 1, NCDHW_ELEM * sizeof(DT));
        pipe.InitBuffer(midBuf, MID_BYTES);
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.AllocTensor<DT>();
        AscendC::DataCopy(s, srcGm, NCDHW_ELEM);
        inQueueSrc.EnQue(s);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> s = inQueueSrc.DeQue<DT>();
        AscendC::LocalTensor<DT> d = outQueueDst.AllocTensor<DT>();
        AscendC::LocalTensor<DT> mid = midBuf.Get<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

#if defined(__NPU_ARCH__)
        // NCDHW shape (5 dims) and NDC1HWC0 shape (6 dims). Strides are unused by the 3510
        // reorder computation but must keep matching dim counts for the static_asserts.
        auto ncdhwShape = AscendC::MakeShape(N, C, D, H, W);
        auto ncdhwStride = AscendC::MakeStride(C * D * H * W, D * H * W, H * W, W, 1);
        auto sixHdShape = AscendC::MakeShape(N, D, C1, H, W, C0);
        auto sixHdStride = AscendC::MakeStride(D * C1 * HWC0, C1 * HWC0, HWC0, W * C0, C0, 1);

        auto ncdhwLayout = AscendC::MakeLayout(ncdhwShape, ncdhwStride);
        auto sixHdLayout = AscendC::MakeLayout(sixHdShape, sixHdStride);

        // forward: NCDHW -> NDC1HWC0
        AscendC::TransDataParams<decltype(ncdhwLayout), decltype(sixHdLayout)> fwdParams{
            ncdhwLayout, sixHdLayout};
        AscendC::TransData<CFG_FWD, DT>(mid, s, tmp, fwdParams);
        AscendC::PipeBarrier<PIPE_V>();

        // backward: NDC1HWC0 -> NCDHW
        AscendC::TransDataParams<decltype(sixHdLayout), decltype(ncdhwLayout)> bwdParams{
            sixHdLayout, ncdhwLayout};
        AscendC::TransData<CFG_BWD, DT>(d, mid, tmp, bwdParams);
#endif

        outQueueDst.EnQue<DT>(d);
        inQueueSrc.FreeTensor(s);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> d = outQueueDst.DeQue<DT>();
        AscendC::DataCopy(dstGm, d, NCDHW_ELEM);
        outQueueDst.FreeTensor(d);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueSrc;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueDst;
    AscendC::TBuf<AscendC::TPosition::VECCALC> midBuf, tmpBuf;
    AscendC::GlobalTensor<DT> srcGm, dstGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR src, GM_ADDR dst)
{
    KernelTransData op;
    op.Init(src, dst);
    op.Process();
}
