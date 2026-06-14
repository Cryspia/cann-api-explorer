# Ascend C · LeakyRelu
- vector / activation (tensor + scalar slope)　- `AscendC::LeakyRelu`　- include: `kernel_operator.h`
- `LeakyRelu(dst, src, scalarValue, count)`: **dst[i] = src[i] >= 0 ? src[i] : scalarValue * src[i]**.
- Supported dtypes: `half`, `float`. Here `float`, negSlope=0.1.
- Example: src=-2 -> -0.2, src=3 -> 3. tol 5e-3, errors=0.
