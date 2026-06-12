/*
 * Hand-written sample: deterministic-compute sync InitDetermineComputeWorkspace / WaitPreBlock / NotifyNextBlock.
 * Used to ensure multiple cores execute a section in blockIdx order (deterministic result).
 * Chain 0->1->...->7: core i first WaitPreBlock to wait for its predecessor, reads chain[i-1] and writes chain[i]=chain[i-1]+1, then NotifyNextBlock.
 * Expect chain[i]=i+1 (core 7=8).
 */
#include "kernel_operator.h"

constexpr int32_t SLOT = 8;
using DT = int32_t;

class KernelDetermineSync {
public:
    __aicore__ inline KernelDetermineSync() {}
    __aicore__ inline void Init(GM_ADDR chain, GM_ADDR sync)
    {
        chainGm.SetGlobalBuffer((__gm__ DT *)chain, 64);
        syncGm.SetGlobalBuffer((__gm__ DT *)sync, 64);
        pipe.InitBuffer(ubSync, 256);
        pipe.InitBuffer(ubData, 32);
    }
    __aicore__ inline void Process()
    {
        int32_t idx = AscendC::GetBlockIdx();
        int32_t num = AscendC::GetBlockNum();
        AscendC::LocalTensor<DT> ubWs = ubSync.Get<DT>();
        AscendC::LocalTensor<DT> data = ubData.Get<DT>();

        AscendC::InitDetermineComputeWorkspace(syncGm, ubWs);

        if (idx > 0) {
            AscendC::WaitPreBlock(syncGm, ubWs);
        }
        int32_t prev = 0;
        if (idx > 0) {
            AscendC::DataCopy(data, chainGm[(idx - 1) * SLOT], SLOT);
            AscendC::PipeBarrier<PIPE_ALL>();
            prev = data.GetValue(0);
        }
        int32_t myval = prev + 1;
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::Duplicate<DT>(data, myval, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();
        AscendC::DataCopy(chainGm[idx * SLOT], data, SLOT);
        AscendC::PipeBarrier<PIPE_ALL>();

        if (idx < num - 1) {
            AscendC::NotifyNextBlock(syncGm, ubWs);
        }
    }
private:
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECCALC> ubSync, ubData;
    AscendC::GlobalTensor<DT> chainGm, syncGm;
};

extern "C" __global__ __aicore__ void k_custom(GM_ADDR chain, GM_ADDR sync)
{
    KernelDetermineSync op;
    op.Init(chain, sync);
    op.Process();
}
