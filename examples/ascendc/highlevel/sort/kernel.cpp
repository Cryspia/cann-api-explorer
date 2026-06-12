/*
 * Hand-written high-level unit: Sort (adv_api/sort, tiling-free count mode).
 * Sort<T,U>(dst, dstIdx, src, srcIdx, tmp, calCount): sort the first calCount elements by value in ascending order.
 * src=[31..0], idx=[0..31] -> dst=[0..31], dstIdx=[31..0]. 32 elements per region.
 */
#include "kernel_operator.h"
#include "lib/sort/sort.h"

constexpr uint32_t N = 32;
constexpr uint32_t TMP_BYTES = 8192;
using DT = float;
using IT = uint32_t;

class KernelSort {
public:
    __aicore__ inline KernelSort() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR xidx, GM_ADDR z, GM_ADDR zidx)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        xidxGm.SetGlobalBuffer((__gm__ IT *)xidx, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        zidxGm.SetGlobalBuffer((__gm__ IT *)zidx, N);
        pipe.InitBuffer(inX, 1, N * sizeof(DT));
        pipe.InitBuffer(inIdx, 1, N * sizeof(IT));
        pipe.InitBuffer(outV, 1, N * sizeof(DT));
        pipe.InitBuffer(outI, 1, N * sizeof(IT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::LocalTensor<IT> iL = inIdx.AllocTensor<IT>();
        AscendC::DataCopy(xL, xGm, N);
        AscendC::DataCopy(iL, xidxGm, N);
        inX.EnQue(xL); inIdx.EnQue(iL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<IT> iL = inIdx.DeQue<IT>();
        AscendC::LocalTensor<DT> vL = outV.AllocTensor<DT>();
        AscendC::LocalTensor<IT> oL = outI.AllocTensor<IT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();
        AscendC::Sort<DT, IT>(vL, oL, xL, iL, tmp, N);
        outV.EnQue<DT>(vL); outI.EnQue<IT>(oL);
        inX.FreeTensor(xL); inIdx.FreeTensor(iL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> vL = outV.DeQue<DT>();
        AscendC::LocalTensor<IT> oL = outI.DeQue<IT>();
        AscendC::DataCopy(zGm, vL, N);
        AscendC::DataCopy(zidxGm, oL, N);
        outV.FreeTensor(vL); outI.FreeTensor(oL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inIdx;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outV, outI;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> xGm, zGm;
    AscendC::GlobalTensor<IT> xidxGm, zidxGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR xidx, GM_ADDR z, GM_ADDR zidx)
{
    KernelSort op;
    op.Init(x, xidx, z, zidx);
    op.Process();
}
