# Ascend C · IBSet / IBWait (cross-core one-to-one flag sync)

- Category: sync/
- APIs covered: `AscendC::IBSet` / `AscendC::IBWait` + `GetBlockIdx` / `GetBlockNum`
- include: `kernel_operator.h`
- Source: CANN 9.1.0 Ascend C API Reference / IBSet, IBWait

## Function
A finer-grained **one-to-one / producer-consumer** cross-core sync than `SyncAll` (the all-core barrier). Both operate on the **same GM slot** `gmWorkspace[blockNum*8*eventID + blockIdx*8]` (each core occupies 32B per event):
- `IBSet<isAIVOnly>(gmWorkspace, ubWorkspace, blockIdx, eventID)`: poll the slot, set it to `1` when `==0` (assert, declaring "I am done").
- `IBWait<isAIVOnly>(gmWorkspace, ubWorkspace, blockIdx, eventID)`: poll the slot, set it to `0` when `==1` (consume, wait for the corresponding core to assert).
- Pairing relies on `blockIdx` (the slot number, conventionally the producer core's idx); `eventID` distinguishes multiple independent events.

## Key points
- Must launch multiple cores (`ACLRT_LAUNCH_KERNEL(k)(blockDim,...)`); `gmWorkspace` must be zeroed on the host (initial 0).
- `ubWorkspace` >= 32B (the impl internally polls by DataCopy of one 32B block).
- The impl internally includes `pipe_barrier(PIPE_ALL)` + MTE2/MTE3 flag; cross-core data (chain) is still read/written to GM via `DataCopy`.
- Difference from `SyncAll`: SyncAll is a symmetric "all cores arrive" barrier; IBSet/IBWait is a directed "A notifies B" sync, which can build pipelines / chained dependencies.

## Simplest example design (chained accumulation, verifying strict ordering)
- Launch 8 cores, chain `0->1->...->7`:
  - Core i: if i>0, first `IBWait(slot i-1)` to wait for core i-1; read `chain[i-1]` and write `chain[i]=chain[i-1]+1` (core 0's predecessor is treated as 0 -> writes 1); if i<7, then `IBSet(slot i)` to notify core i+1.
- Expect `chain[i]=i+1` (core 7=8). **If IBWait fails, core i reads the predecessor's not-yet-written stale value (-1) -> wrong result**.
- The host verifies errors=0 (instr approx 7956, 8 cores).
