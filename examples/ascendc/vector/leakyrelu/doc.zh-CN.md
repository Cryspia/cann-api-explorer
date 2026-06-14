# Ascend C · LeakyRelu
- 矢量 / 激活函数（tensor + 标量斜率）　- `AscendC::LeakyRelu`　- include：`kernel_operator.h`
- `LeakyRelu(dst, src, scalarValue, count)`：**dst[i] = src[i] >= 0 ? src[i] : scalarValue * src[i]**。
- 支持的 dtype：`half`、`float`。本例用 `float`，负斜率=0.1。
- 例：src=-2 → -0.2，src=3 → 3。容差 5e-3，errors=0。
