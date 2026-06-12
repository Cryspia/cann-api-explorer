# Ascend C · SyncAll (multi-core sync barrier)

- Category: sync/
- APIs covered: `AscendC::SyncAll` + `GetBlockIdx` / `GetBlockNum`
- include: `kernel_operator.h`
- Source: CANN 9.1.0 Ascend C API Reference / SyncAll

## Function
Cross-core barrier: all participating cores continue together only after every one of them has reached `SyncAll`. Two overloads:
- `SyncAll<isAIVOnly>(gmWorkspace, ubWorkspace, usedCores)`: a **software barrier**, using a GM counter + dcci for cache flush/invalidate, general-purpose.
- `SyncAll<isAIVOnly, config>()` (3510/5102): a **hardware FFTS** cross-core sync instruction (`ffts_cross_core_sync`), requiring no workspace.

This example uses the software version (with built-in GM consistency handling, the most robust).

## Key point 1: true multi-core requires launching blockDim>1
The first argument of `ACLRT_LAUNCH_KERNEL(k_custom)(blockDim, stream, ...)` is the number of cores; inside the kernel, `GetBlockIdx()` returns 0..blockDim-1 and `GetBlockNum()` returns blockDim. The local simulation has 20 cores available; this example uses 8.

## Key point 2: cross-core GM communication must use DataCopy, not scalar SetValue
A scalar `GlobalTensor::SetValue` write to GM lingers in cache and is not guaranteed to write back to DDR -> the host memcpy reads stale values (in practice, flag stays at its initial value -1).
Instead, write with `DataCopy(UB->GM)` (via MTE3, guaranteeing write-back) and read with `DataCopy(GM->UB)`; insert `PipeBarrier<PIPE_ALL>()` between scalar operations (`GetValue`) and the data movement to order them.
The SyncAll software barrier flushes/invalidates GM at the barrier point, ensuring phase 2 reads the phase-1 writes of other cores.

## Simplest example design (verifying sync semantics)
- Launch 8 cores: each core writes `idx+1` via DataCopy into the idx-th 32B slot (SLOT=8 int32 aligned).
- `SyncAll(syncGm, ubWs, num)` barrier.
- Each core reads back all 8 slots via DataCopy and sums them -> `result[idx]`.
- Expect: `flag[i]=i+1`, and **all cores** have `result[i]=1+..+8=36` (if the barrier fails, some core reads a not-yet-written slot, sum != 36).
- The host verifies errors=0 (instr approx 6367, 8 cores).
