# Ascend C · Matmul (Cube, with TCubeTiling)

- Category: advanced cube / matrix multiply
- APIs covered: `matmul::Matmul` object, `REGIST_MATMUL_OBJ`, `SetTensorA/SetTensorB`, `IterateAll`; host `matmul_tiling::MatmulApiTiling`
- include: kernel `lib/matmul_intf.h`; host `tiling/matrix/matmul_tiling.h`
- Source: CANN 9.1.0 Ascend C API Reference / Matmul advanced API

## Function
`C[M,N] = A[M,K] @ B[K,N]`, A/B = half, C = float. The Cube unit performs the MAD.

## Key point: host computes TCubeTiling -> passed in via GM -> kernel REGIST
Matmul is a Cube operator; it has many tiling fields (50+) that depend on the L1/L0/UB layout, so they **must be computed on the host side**:
```cpp
// host (main.cpp)
matmul_tiling::MatmulApiTiling t;
t.SetAType(GM, ND, DT_FLOAT16); t.SetBType(GM, ND, DT_FLOAT16);
t.SetCType(GM, ND, DT_FLOAT);   t.SetBiasType(GM, ND, DT_FLOAT);
t.SetShape(M,N,K); t.SetOrgShape(M,N,K); t.SetBias(false); t.SetBufferSpace(-1,-1,-1);
AscendC::tiling::TCubeTiling tiling;   // ★ must use the same struct as the kernel (see the "pitfall" below)
t.GetTiling(tiling);                   // copy to device, passed as the kernel's 5th argument
```
```cpp
// kernel (kernel.cpp)
using AType = MatmulType<TPosition::GM, CubeFormat::ND, half>;  // B/C/Bias types are analogous
TCubeTiling tiling;  /* copied in from GM uint32 by uint32 */
Matmul<AType,BType,CType,BiasType> mm;
REGIST_MATMUL_OBJ(&pipe, workspace /*built-in GM workspace*/, mm, &tiling);
mm.SetTensorA(aG); mm.SetTensorB(bG); mm.IterateAll(cG); mm.End();
```

## Pitfall record (important)
On the host side you **must use `GetTiling(AscendC::tiling::TCubeTiling&)`**, not `optiling::TCubeTiling`.
The latter is defined with macros (TILING_DATA_FIELD_DEF) and has a **different memory layout** from the `AscendC::tiling::TCubeTiling` (a plain struct with 50 fields) that the kernel reads;
mixing them causes the kernel to read misaligned zero values -> in the simulation, `scalar_rem: div by 0!` + `pem_lsu mem_map unrecognize ldst addr` (addresses computed as 0/garbage), spinning into a deadlock. It works once switched to the same struct.

## Simplest example design
- `M=N=K=64`, A/B = half all `1.0` (0x3C00) -> C = float all `K=64`.
- The host verifies C==64 (errors=0); the build links `libtiling_api.a` + `libplatform.so`.
- See `kernel.cpp` / `main.cpp` / `CMakeLists.txt`; for the result see `RESULT.md`. Instruction count 5245.
