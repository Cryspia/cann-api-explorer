# Ascend C · Transpose (16x16 block transpose)

- Category: vector compute / data transform (basic API)
- API covered: `Transpose` (hardware `vtranspose`, b16 16x16)

## Functionality
`Transpose(dst, src)`: `dst[i][j] = src[j][i]`, does a hardware transpose of a 16x16 b16 (half/uint16) block.

## Function prototype
```cpp
template <typename T> __aicore__ inline void Transpose(const LocalTensor<T>& dst, const LocalTensor<T>& src);
```
- On this arch (3510) it goes through `TransposeImpl` -> `vtranspose`, supporting only b16, fixed 16x16.
- For more general shapes use the overload with `TransposeParamsExt` or the adv_api `ConfusionTranspose` (requires tiling).

## Minimal example design
- 16x16 half, `src[r][c]=r*16+c` (0..255, exact in half). After transpose `dst[r][c]=c*16+r`.
- host uses minimal half encode/decode to verify element by element. See `kernel.cpp`/`main.cpp`, results in `RESULT.md`.
