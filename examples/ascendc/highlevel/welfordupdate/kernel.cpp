/*
 * Hand-written high-level unit: WelfordUpdate (adv_api normalization, the online step).
 * One Welford update step that folds a new sample x into running mean/variance accumulators.
 * 3510 per-element formula (float path), from welford_3510_impl.h:
 *   tmp     = x - inMean
 *   outMean = inMean + nRec * tmp          (nRec = 1/n, the running count reciprocal)
 *   outVar  = inVar  + tmp * (x - outMean) (running M2 accumulation)
 * This complements the existing welfordfinalize unit (which merges block accumulators).
 *
 * Test (n=2): inMean=1, inVar=0, x=3, nRec=0.5
 *   tmp     = 3 - 1 = 2
 *   outMean = 1 + 0.5*2 = 2
 *   outVar  = 0 + 2*(3-2) = 2
 * Single tile of ELEM=8 float elements (32B), abLength=abComputeLength=ELEM.
 */
#include "kernel_operator.h"
#include "lib/normalization/layernorm.h"

constexpr uint32_t ELEM = 8;       // single tile, 8 floats = 32B
using XT = float;                  // inputX type (half/float supported)
using UT = float;                  // mean/variance accumulator type (always float)

class KernelWelfordUpdate {
public:
    __aicore__ inline KernelWelfordUpdate() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR inMean, GM_ADDR inVar, GM_ADDR outMean, GM_ADDR outVar)
    {
        xGm.SetGlobalBuffer((__gm__ XT *)x, ELEM);
        inMeanGm.SetGlobalBuffer((__gm__ UT *)inMean, ELEM);
        inVarGm.SetGlobalBuffer((__gm__ UT *)inVar, ELEM);
        outMeanGm.SetGlobalBuffer((__gm__ UT *)outMean, ELEM);
        outVarGm.SetGlobalBuffer((__gm__ UT *)outVar, ELEM);
        pipe.InitBuffer(inX, 1, ELEM * sizeof(XT));
        pipe.InitBuffer(inM, 1, ELEM * sizeof(UT));
        pipe.InitBuffer(inV, 1, ELEM * sizeof(UT));
        pipe.InitBuffer(outM, 1, ELEM * sizeof(UT));
        pipe.InitBuffer(outV, 1, ELEM * sizeof(UT));
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<XT> xL = inX.AllocTensor<XT>();
        AscendC::LocalTensor<UT> mL = inM.AllocTensor<UT>();
        AscendC::LocalTensor<UT> vL = inV.AllocTensor<UT>();
        AscendC::DataCopy(xL, xGm, ELEM);
        AscendC::DataCopy(mL, inMeanGm, ELEM);
        AscendC::DataCopy(vL, inVarGm, ELEM);
        inX.EnQue(xL); inM.EnQue(mL); inV.EnQue(vL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<XT> xL = inX.DeQue<XT>();
        AscendC::LocalTensor<UT> mL = inM.DeQue<UT>();
        AscendC::LocalTensor<UT> vL = inV.DeQue<UT>();
        AscendC::LocalTensor<UT> omL = outM.AllocTensor<UT>();
        AscendC::LocalTensor<UT> ovL = outV.AllocTensor<UT>();

        AscendC::WelfordUpdateParam para;
        para.rnLength = ELEM;
        para.abLength = ELEM;
        para.abComputeLength = ELEM;     // number of elements processed (K)
        para.nRec = 0.5f;                // 1/n with n=2

        AscendC::WelfordUpdate<XT, UT>(omL, ovL, mL, vL, xL, para);

        outM.EnQue<UT>(omL); outV.EnQue<UT>(ovL);
        inX.FreeTensor(xL); inM.FreeTensor(mL); inV.FreeTensor(vL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<UT> omL = outM.DeQue<UT>();
        AscendC::LocalTensor<UT> ovL = outV.DeQue<UT>();
        AscendC::DataCopy(outMeanGm, omL, ELEM);
        AscendC::DataCopy(outVarGm, ovL, ELEM);
        outM.FreeTensor(omL); outV.FreeTensor(ovL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inX, inM, inV;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outM, outV;
    AscendC::GlobalTensor<XT> xGm;
    AscendC::GlobalTensor<UT> inMeanGm, inVarGm, outMeanGm, outVarGm;
};

extern "C" __global__ __aicore__ void k_custom(
    GM_ADDR x, GM_ADDR inMean, GM_ADDR inVar, GM_ADDR outMean, GM_ADDR outVar)
{
    KernelWelfordUpdate op;
    op.Init(x, inMean, inVar, outMean, outVar);
    op.Process();
}
