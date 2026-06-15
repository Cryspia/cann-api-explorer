> 中文版: [docs_zh-CN/notcovered.md](../docs_zh-CN/notcovered.md)

# Track C · Not-covered libraries (why not covered + conditions needed for future coverage)

> **Premise**: This project uses **cannsim card-free (no-NPU) simulation (CAModel)** to run Ascend C kernels. CAModel only performs instruction-level simulation and produces reports for **device instructions that are compiled into the kernel and run on the AI Core**. The libraries below are either **host-side upper-layer frameworks** (do not produce AI Core instructions) or **need a real driver / multi-NPU / dedicated hardware units**, so they are out of scope for card-free (no-NPU) simulation. This machine has only the toolkit installed, no NPU driver, so these are documentation only, left for the future when a real environment is available.

| Library | What it is | Why not covered | Conditions needed for future coverage |
|----|--------|-----------|-------------------|
| **GE** (Graph Engine) | compiles, optimizes, schedules, and executes the whole compute graph | a host-side graph compilation/scheduling framework, does not produce AI Core instruction simulation; depends on a full runtime + graph compiler + driver | real NPU + driver + GE runtime; verification is end-to-end graph execution, not instruction simulation |
| **aolapi / AOL / aclnn** (operator acceleration library) | precompiled high-level operator library invoked via the `aclnn*` host API (analogous to cuDNN); 350+ ops such as `aclnnConvolution` / `aclnnFlashAttention` | host-side loading/calling of precompiled operator binaries, does not expose simulatable kernel source — a **different layer** from the Ascend C **kernel** APIs of Track A; the `aclnnop` headers ship in a separate operator-library (nnal/opp) package, not in the toolkit | real device + operator binary package; calling it is a black box, no instruction-level trace |
| **HCCL** (collective communication library) | multi-NPU AllReduce/AllGather/Broadcast/ReduceScatter etc. | needs **multiple NPUs** + inter-card network (HCCS/RoCE), single-machine card-free (no-NPU) simulation has no peer | ≥2 real NPUs + interconnect network; verify collective communication correctness and bandwidth |
| **HIXL** (heterogeneous interconnect exchange layer) | interconnect and data exchange across devices / heterogeneous units | needs multi-device interconnect hardware, card-free (no-NPU) simulation has no interconnect topology | multi-device interconnect hardware environment |
| **ATB** (Ascend Transformer Boost) | Transformer high-level acceleration library (attention/FFN/PagedAttention etc. wrappers) | host-side high-level wrapper, internally calls runtime + operator library; does not expose a single simulatable kernel | real device + runtime + operator library; end-to-end model inference verification |
| **SiP** (System-in-Package related features) | package-level system / dedicated hardware feature interfaces | depends on specific package hardware features, not provided by card-free (no-NPU) simulation | corresponding package hardware |
| **DVPP** (Digital Vision Pre-Processing) | image/video encode-decode, resize, color conversion, etc. hardware units | needs a **dedicated DVPP hardware unit** (not the AI Core), CAModel does not simulate that unit | real-device DVPP unit; verify encode-decode/pre-processing output |

## Relationship to the three covered tracks

- **Track A (completed, 172/172)**: Ascend C kernel API —— the sole target of CAModel instruction-level simulation, the core of this project.
- **Track B (completed)**: Runtime/AscendCL host API —— scaffolding for launching kernels, see [`runtime/host_api.md`](runtime/host_api.md), does not produce instruction reports but its correctness is implicit in the 172 PASSED units of Track A.
- **Track C (this document)**: upper-layer frameworks / hardware-requiring libraries —— explicitly out of scope for card-free (no-NPU) simulation, recording the boundary and future conditions.

> The boundary in one sentence: **what can compile into AI Core instructions and be simulated into a report by CAModel → exhausted by Track A; host scaffolding → recorded by Track B; everything else (upper-layer frameworks / hardware-requiring) → this Track C document explains why it is not covered.**
