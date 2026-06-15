# Ascend C · Compares
- vector / comparison (vector vs scalar)　- `AscendC::Compares` (+ Select for verification)　- include: `kernel_operator.h`
- `Compares<T,U>(maskDst, src0, src1Scalar, CMPMODE, count)`: compares each element of a vector against a **scalar**, writing the result to a bit mask. `Compares` is the current name; the header marks `CompareScalar` as deprecated ("CompareScalar has been updated, please use Compares instead") — they share the scalar-comparison semantics but `Compares` is the canonical API. This differs from `Compare`, which compares two vectors element by element.
- Example: src0[i]=i, scalar=31.5, GT -> mask=(i>=32); Select converts to float z[i]=(i>=32)?1:0. errors=0.
