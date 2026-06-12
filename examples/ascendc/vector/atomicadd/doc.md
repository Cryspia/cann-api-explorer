# Ascend C · SetAtomicAdd (atomic add to GM)
- Category: atomic / multi-core　- API: `AscendC::SetAtomicAdd` / `SetAtomicNone`　- include: `kernel_operator.h`
- `SetAtomicAdd<T>()` sets the subsequent `DataCopy(UB->GM)` to **atomic add** mode; `SetAtomicNone()` restores normal writes.
- Multi-core accumulation scenario: launch 8 cores, each core does `SetAtomicAdd` then DataCopy of all 1.0 into the same GM region -> each element = 8.0 (**GM must be zeroed by the host**).
- Example: errors=0 (instr≈1760, 8 cores). SetAtomicMax/Min are also available.
