/*
 * Hand-written sample: IBSet / IBWait (cross-core one-to-one flag sync, finer-grained than the SyncAll full barrier).
 * Semantics (see impl): IBSet/IBWait operate on the same GM slot gmWorkspace[blockNum*8*eventID + blockIdx*8]:
 *   - IBSet(.., slot, eid)  : poll the slot, set it to 1 when ==0 (assert, meaning "I am done")
 *   - IBWait(.., slot, eid) : poll the slot, set it to 0 when ==1 (consume, wait for the corresponding core to assert)
 * Chained accumulation 0->1->...->7: core i first IBWait(slot i-1) to wait for core i-1, reads chain[i-1] and writes chain[i]=chain[i-1]+1,
 * then IBSet(slot i) to notify core i+1. Finally chain[i]=i+1 (core 7=8). If IBWait fails, core i reads the predecessor's stale value.
 */
#include "kernel_operator.h"

constexpr int32_t SLOT = 8;       // 32B slot per core
constexpr int32_t EVENT = 0;
using DT = int32_t;

class KernelIBSetWait {
public:
    __aicore__ inline KernelIBSetWait() {}
    __aicore__ inline void Init(GM_ADDR chain, GM_ADDR sync)
    {
        chainGm.SetGlobalBuffer((__gm__ DT *)chain, 64);
        syncGm.SetGlobalBuffer((__gm__ DT *)sync, 64);
        pipe.InitBuffer(ubSync, 32);    // ubWorkspace for IBSet/IBWait
        pipe.InitBuffer(ubData, 32);    // for reading/writing chain
    }
    __aicore__ inline void Process()
    {
        int32_t idx = AscendC::GetBlockIdx();
        int32_t num = AscendC::GetBlockNum();
        AscendC::LocalTensor<DT> ubWs = ubSync.Get<DT>();
        AscendC::LocalTensor<DT> data = ubData.Get<DT>();

        // Wait for the predecessor core to finish
        if (idx > 0) {
            AscendC::IBWait<true>(syncGm, ubWs, idx - 1, EVENT);
        }

        // Read predecessor value + 1 (core 0's predecessor is treated as 0)
        int32_t prev = 0;
        if (idx > 0) {
            AscendC::DataCopy(data, chainGm[(idx - 1) * SLOT], SLOT);
            AscendC::PipeBarrier<PIPE_ALL>();
            prev = data.GetValue(0);
        }
        int32_t myval = prev + 1;

        // Write its own slot
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::Duplicate<DT>(data, myval, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::DataCopy(chainGm[idx * SLOT], data, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();

        // Notify the successor core
        if (idx < num - 1) {
            AscendC::IBSet<true>(syncGm, ubWs, idx, EVENT);
        }
    }
private:
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECCALC> ubSync, ubData;
    AscendC::GlobalTensor<DT> chainGm, syncGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR chain, GM_ADDR sync)
{
    KernelIBSetWait op;
    op.Init(chain, sync);
    op.Process();
}
