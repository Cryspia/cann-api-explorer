/*
 * Hand-written high-level unit: WelfordFinalize (adv_api, this arch=3510).
 * Combines per-partition (partial) means and variances into the final mean and variance
 * using the Welford algorithm. Uses the no-counts overload:
 *   WelfordFinalize(outputMean, outputVariance, inputMean, inputVariance, sharedTmpBuffer, para)
 * Inputs inputMean/inputVariance have shape [abLength]; outputs are scalar (written at index 0).
 *
 * Para meaning (3510): abLength = headCountLength + tailCountLength (32B aligned).
 * We take the simplest path tailCountLength=0 (branch 1 of WelfordFinalizeForB32):
 *   outMean = abRec * sum(inputMean)              (over abLength partitions)
 *   outVar  = rRec  * sum(inputVariance + rnLength*(inputMean - outMean)^2)
 * With every partition mean equal (M) and variance equal (V), and abRec = rRec = 1/abLength:
 *   outMean = M, and (inputMean - outMean) = 0 -> outVar = V.
 * Design: abLength=8 (<64 simple reduce path), all inputMean=4 -> outMean=4; all inputVariance=9 -> outVar=9.
 */
#include "kernel_operator.h"
#include "lib/normalization/welfordfinalize.h"

constexpr uint32_t AB = 8;              // abLength: number of partitions (8 floats = 32B aligned, < 64)
constexpr uint32_t TMP_BYTES = 16384;
using DT = float;

class KernelWelfordFinalize {
public:
    __aicore__ inline KernelWelfordFinalize() {}
    __aicore__ inline void Init(GM_ADDR inMean, GM_ADDR inVar, GM_ADDR outMean, GM_ADDR outVar)
    {
        inMeanGm.SetGlobalBuffer((__gm__ DT *)inMean, AB);
        inVarGm.SetGlobalBuffer((__gm__ DT *)inVar, AB);
        outMeanGm.SetGlobalBuffer((__gm__ DT *)outMean, AB);
        outVarGm.SetGlobalBuffer((__gm__ DT *)outVar, AB);

        pipe.InitBuffer(inQueueM, 1, AB * sizeof(DT));
        pipe.InitBuffer(inQueueV, 1, AB * sizeof(DT));
        pipe.InitBuffer(outQueueM, 1, AB * sizeof(DT));
        pipe.InitBuffer(outQueueV, 1, AB * sizeof(DT));
        pipe.InitBuffer(tmpBuf, TMP_BYTES);
    }
    __aicore__ inline void Process() { CopyIn(); Compute(); CopyOut(); }
private:
    __aicore__ inline void CopyIn()
    {
        AscendC::LocalTensor<DT> mL = inQueueM.AllocTensor<DT>();
        AscendC::LocalTensor<DT> vL = inQueueV.AllocTensor<DT>();
        AscendC::DataCopy(mL, inMeanGm, AB);
        AscendC::DataCopy(vL, inVarGm, AB);
        inQueueM.EnQue(mL); inQueueV.EnQue(vL);
    }
    __aicore__ inline void Compute()
    {
        AscendC::LocalTensor<DT> mL = inQueueM.DeQue<DT>();
        AscendC::LocalTensor<DT> vL = inQueueV.DeQue<DT>();
        AscendC::LocalTensor<DT> outML = outQueueM.AllocTensor<DT>();
        AscendC::LocalTensor<DT> outVL = outQueueV.AllocTensor<DT>();
        AscendC::LocalTensor<uint8_t> tmp = tmpBuf.Get<uint8_t>();

        AscendC::WelfordFinalizePara para;
        para.rnLength = AB;                 // weight for (mean - outMean)^2; zero here since means are equal
        para.abLength = AB;                 // total partitions, 32B aligned
        para.headCount = AB;                // not used by the no-counts tailCountLength==0 branch
        para.headCountLength = AB;          // head block length = abLength (tail is empty)
        para.tailCount = 1;                 // non-zero so the assert "tailCountLength!=0 when tailCount==0" is satisfied
        para.tailCountLength = 0;           // empty tail -> branch 1 (simple reduce)
        para.abRec = 1.0f / (float)AB;      // mean coefficient 1/abLength
        para.rRec = 1.0f / (float)AB;       // variance coefficient 1/abLength
        para.rRecWithCorrection = 1.0f / (float)AB;

        // isReuseSource=false, default config (isCorrection=false).
        AscendC::WelfordFinalize(outML, outVL, mL, vL, tmp, para);

        outQueueM.EnQue<DT>(outML);
        outQueueV.EnQue<DT>(outVL);
        inQueueM.FreeTensor(mL);
        inQueueV.FreeTensor(vL);
    }
    __aicore__ inline void CopyOut()
    {
        AscendC::LocalTensor<DT> outML = outQueueM.DeQue<DT>();
        AscendC::LocalTensor<DT> outVL = outQueueV.DeQue<DT>();
        AscendC::DataCopy(outMeanGm, outML, AB);
        AscendC::DataCopy(outVarGm, outVL, AB);
        outQueueM.FreeTensor(outML);
        outQueueV.FreeTensor(outVL);
    }

    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueM, inQueueV;
    AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueM, outQueueV;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuf;
    AscendC::GlobalTensor<DT> inMeanGm, inVarGm, outMeanGm, outVarGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR inMean, GM_ADDR inVar, GM_ADDR outMean, GM_ADDR outVar)
{
    KernelWelfordFinalize op;
    op.Init(inMean, inVar, outMean, outVar);
    op.Process();
}
