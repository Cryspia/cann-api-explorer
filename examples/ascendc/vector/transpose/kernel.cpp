/*
 * Hand-written unit: Transpose (basic API, 16x16 b16 block transpose, hardware vtranspose).
 * dst[i][j] = src[j][i]. The simple Transpose<half>(dst,src) on this arch=3510 -> vtranspose.
 */
#include "kernel_operator.h"

constexpr uint32_t H = 16;
constexpr uint32_t W = 16;
constexpr uint32_t N = H * W;     // 256
using DT = half;

class KernelTranspose {
public:
    __aicore__ inline KernelTranspose() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR z)
    {
        xGm.SetGlobalBuffer((__gm__ DT *)x, N);
        zGm.SetGlobalBuffer((__gm__ DT *)z, N);
        pipe.InitBuffer(inQueueX, 1, N * sizeof(DT));
        pipe.InitBuffer(outQueueZ, 1, N * sizeof(DT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.AllocTensor<DT>();
        AscendC::DataCopy(xL, xGm, N);
        inQueueX.EnQue(xL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> xL = inQueueX.DeQue<DT>();
        AscendC::LocalTensor<DT> zL = outQueueZ.AllocTensor<DT>();
        AscendC::Transpose(zL, xL);     // 16x16 b16 transpose
        outQueueZ.EnQue<DT>(zL);
        inQueueX.FreeTensor(xL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> zL = outQueueZ.DeQue<DT>();
        AscendC::DataCopy(zGm, zL, N);
        outQueueZ.FreeTensor(zL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ;
    AscendC::GlobalTensor<DT> xGm, zGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR x, GM_ADDR z)
{
    KernelTranspose op;
    op.Init(x, z);
    op.Process();
}
