/*
 * Hand-written sample: multi-core sync SyncAll (cross-core barrier).
 * launch blockDim=8: each core writes (idx+1) via DataCopy into the idx-th 32B slot of flagGm,
 * after the SyncAll barrier each core reads back all slots via DataCopy and sums them, writing resultGm. With sync in effect -> each core's sum = 1+..+8 = 36.
 * Key point: GM reads/writes use DataCopy (via MTE, guaranteeing write-back/visibility), not scalar SetValue (which lingers in cache, invisible to the host).
 * The SyncAll(gmWorkspace, ubWorkspace, usedCores) software barrier includes a GM cache flush; PipeBarrier orders scalar operations against the data movement.
 */
#include "kernel_operator.h"

constexpr int32_t N = 8;       // number of cores
constexpr int32_t SLOT = 8;    // 8 int32 per core = 32B-aligned slot
using DT = int32_t;

class KernelSyncAll {
public:
    __aicore__ inline KernelSyncAll() {}
    __aicore__ inline void Init(GM_ADDR flag, GM_ADDR result, GM_ADDR sync)
    {
        flagGm.SetGlobalBuffer((__gm__ DT *)flag, N * SLOT);
        resultGm.SetGlobalBuffer((__gm__ DT *)result, N * SLOT);
        syncGm.SetGlobalBuffer((__gm__ DT *)sync, N);
        pipe.InitBuffer(oneBuf, SLOT * sizeof(DT));
        pipe.InitBuffer(allBuf, N * SLOT * sizeof(DT));
        pipe.InitBuffer(syncUb, 256);
    }
    __aicore__ inline void Process()
    {
        int32_t idx = AscendC::GetBlockIdx();
        int32_t num = AscendC::GetBlockNum();

        // Phase 1: fill UB with idx+1 -> DataCopy writes its own slot
        AscendC::LocalTensor<DT> one = oneBuf.Get<DT>();
        AscendC::Duplicate<DT>(one, idx + 1, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::DataCopy(flagGm[idx * SLOT], one, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();

        // Barrier: all cores arrive (includes GM flush/invalidate)
        AscendC::LocalTensor<DT> ubWs = syncUb.Get<DT>();
        AscendC::SyncAll<true>(syncGm, ubWs, num);

        // Phase 2: read back all slots and sum
        AscendC::LocalTensor<DT> all = allBuf.Get<DT>();
        AscendC::DataCopy(all, flagGm, num * SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();
        int32_t s = 0;
        for (int32_t j = 0; j < num; j++) {
            s += all.GetValue(j * SLOT);
        }
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::Duplicate<DT>(one, s, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::DataCopy(resultGm[idx * SLOT], one, SLOT);
    }
private:
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECCALC> oneBuf, allBuf, syncUb;
    AscendC::GlobalTensor<DT> flagGm, resultGm, syncGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR flag, GM_ADDR result, GM_ADDR sync)
{
    KernelSyncAll op;
    op.Init(flag, result, sync);
    op.Process();
}
