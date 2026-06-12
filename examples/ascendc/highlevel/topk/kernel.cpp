/*
 * Hand-written high-level unit: TopK (adv_api/sort, with TopkTiling, host computes tiling -> passed in via GM).
 * Takes the largest k values and their indices. The host TopKTilingFunc fills AscendC::tiling::TopkTiling.
 * src=[0..31], isLargest=true -> top4 values=[31,30,29,28], indices=[31,30,29,28].
 */
#include "kernel_operator.h"
#include "lib/sort/topk.h"

constexpr int32_t N = 32;       // input length
constexpr int32_t INNER = 32;   // number of 32B-aligned elements of n (float: 32 is already aligned)
constexpr int32_t OUTTER = 1;
constexpr int32_t K = 4;
constexpr uint32_t TMP_BYTES = 8192;
using DT = float;

class KernelTopK {
public:
    __aicore__ inline KernelTopK() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR xidx, GM_ADDR z, GM_ADDR zidx, GM_ADDR tilingGm)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        xidxGm.SetGlobalBuffer((__gm__ int32_t *)xidx, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, INNER);
        zidxGm.SetGlobalBuffer((__gm__ int32_t *)zidx, INNER);
        tGm = tilingGm;
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inIdx, 1, N * sizeof(int32_t));
        pipe.InitBuffer(outV, 1, INNER * sizeof(DT));
        pipe.InitBuffer(outI, 1, INNER * sizeof(int32_t));
        pipe.InitBuffer(finishBuf, 32);
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<int32_t> iL = inIdx.AllocTensor<int32_t>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(iL, xidxGm, N);
        inX.EnQue(xL); inIdx.EnQue(iL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<int32_t> iL = inIdx.DeQue<int32_t>();
        AscendC::LocalTensor<DT> vL = outV.AllocTensor<DT>();
        AscendC::LocalTensor<int32_t> oL = outI.AllocTensor<int32_t>();
        AscendC::LocalTensor<bool> finish = finishBuf.Get<bool>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        // Copy in the host-computed TopkTiling from GM
        AscendC::tiling::TopkTiling t;
        auto src = reinterpret_cast<__gm__ uint32_t *>(tGm);
        auto dst = reinterpret_cast<uint32_t *>(&t);
        for (uint32_t i = 0; i < sizeof(AscendC::tiling::TopkTiling) / sizeof(uint32_t); i++) dst[i] = src[i];

        AscendC::TopKInfo info;
        info.outter = OUTTER; info.inner = INNER; info.n = N;

        // isInitIndex=true (use the srcIndex we provide), isHasfinish=false, TOPK_NORMAL
        AscendC::TopK<DT, true, false, false, AscendC::TopKMode::TOPK_NORMAL>(
            vL, oL, xL, iL, finish, tmp, K, t, info, true);

        outV.EnQue<DT>(vL); outI.EnQue<int32_t>(oL);
        inX.FreeTensor(xL); inIdx.FreeTensor(iL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> vL = outV.DeQue<DT>();
        AscendC::LocalTensor<int32_t> oL = outI.DeQue<int32_t>();
        AscendC::DataCopy(zGm, vL, INNER);
        AscendC::DataCopy(zidxGm, oL, INNER);
        outV.FreeTensor(vL); outI.FreeTensor(oL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inIdx;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outV, outI;
    AscendC::TBuf<AscendC::TPosition::VECCALC> finishBuf, tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
    AscendC::GlobalTensor<int32_t> xidxGm, zidxGm;
    GM_ADDR tGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR xidx, GM_ADDR z, GM_ADDR zidx, GM_ADDR tilingGm)
{
    KernelTopK op;
    op.Init(x, xidx, z, zidx, tilingGm);
    op.Process();
}
