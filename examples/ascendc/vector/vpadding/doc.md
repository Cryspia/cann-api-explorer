# Ascend C · VectorPadding (⚠️ not made into a unit / semantics need deeper investigation)
- vector / boundary padding　- `AscendC::VectorPadding`　- include: `kernel_operator.h`
- `VectorPadding(dst, src, padMode, padSide, count)`.
- **Measured: in count mode with padMode=0 the output is all 0**, hard to interpret as meaningful boundary padding -- its semantics seem to depend on the mask+repeat (level 0) mode together with the precise definition of padMode; the simplified count mode is not enough to express it.
- Already excluded from the count (meta.json.unsupported). The interface code is kept for reference. A different layer from the adv_api `Pad` (already covered).
