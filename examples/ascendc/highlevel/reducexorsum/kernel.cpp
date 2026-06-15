/*
 * Hand-written high-level unit: ReduceXorSum (adv_api reduce_xor_sum).
 * ReduceXorSum<T>(dst, src0, src1, sharedTmpBuffer, calCount):
 *   computes f = sum_i(src0[i] ^ src1[i]) over `calCount` elements (bitwise XOR
 *   per element, then a full reduction sum). Device data type is int16_t only on
 *   this architecture; the scalar result is written to dst[0].
 * Layout: 64 int16 elements per input. The single-element result is copied out as
 *   a full 32-byte aligned block (DataCopy minimum transfer), host reads dst[0].
 */
#include "kernel_operator.h"
#include "adv_api/reduce/reduce_xor_sum.h"

constexpr uint32_t COUNT = 64;
constexpr uint32_t ELEM = 64;          // ELEM*sizeof(int16_t)=128, 32-byte aligned
constexpr uint32_t TMP_BYTES = 16384;
using DT = int16_t;

class KernelReduceXorSum {
public:
    __aicore__ inline KernelReduceXorSum() {}
    __aicore__ inline void Init(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
    {
        x0Gm.SetGlobalBuffer((__gm__ DT *)x0, ELEM);
        x1Gm.SetGlobalBuffer((__gm__ DT *)x1, ELEM);
        zGm.SetGlobalBuffer((__gm__ DT *)z, ELEM);
        pipe.InitBuffer(inX0, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(inX1, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(outZ, 1, ELEM * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> x0L = inX0.AllocTensor<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.AllocTensor<DT>();
        AscendC::DataCopy(x0L, x0Gm, ELEM);
        AscendC::DataCopy(x1L, x1Gm, ELEM);
        inX0.EnQue(x0L);
        inX1.EnQue(x1L);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> x0L = inX0.DeQue<DT>();
        AscendC::LocalTensor<DT> x1L = inX1.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outZ.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::ReduceXorSum<DT>(zL, x0L, x1L, tmp, COUNT);

        outZ.EnQue<DT>(zL);
        inX0.FreeTensor(x0L);
        inX1.FreeTensor(x1L);
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
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX0;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX1;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outZ;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> x0Gm, x1Gm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x0, GM_ADDR x1, GM_ADDR z)
{
    KernelReduceXorSum op;
    op.Init(x0, x1, z);
    op.Process();
}
