/*
 * Hand-written high-level unit: Mean (adv_api reduce, last-axis reduction with MeanParams + tmp).
 * Mean<T>(dst, src, sharedTmpBuffer, MeanParams{outter, inner, n}):
 *   reduces each of `outter` rows (row stride `inner`, valid length `n`) to its average.
 * Layout [outter=1, n=64], inner=64 (inner*sizeof(float)=256, multiple of 32).
 * src all 2.0 -> mean over 64 elements = 2.0, written to dst[0].
 */
#include "kernel_operator.h"
#include "lib/reduce/mean.h"

constexpr uint32_t OUTTER = 1;
constexpr uint32_t N = 64;
constexpr uint32_t INNER = 64;            // inner*sizeof(float)=256, 32-byte aligned
constexpr uint32_t ELEM = OUTTER * INNER; // 64
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelMean {
public:
    __aicore__ inline KernelMean() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inX, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, ELEM);
        inX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::MeanParams params{OUTTER, INNER, N};
        AscendC::Mean<DT>(zL, xL, tmp, params);

        outZ.EnQue<DT>(zL);
        inX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outZ.DeQue<DT>();
        // Result lands in zL[0]; copy the whole ELEM-sized (32-byte aligned) buffer
        // out because DataCopy's minimum transfer is one 32-byte block. Host reads z[0].
        AscendC::DataCopy(zGm, zL, ELEM);
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
    KernelMean op;
    op.Init(x, z);
    op.Process();
}
