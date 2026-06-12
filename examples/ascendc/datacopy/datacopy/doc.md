# Ascend C · DataCopy (data movement)

- Category: data movement (hand-written identity sample)
- APIs covered: `DataCopy` (GM<->Local, Local<->Local)
- Source: CANN 9.1.0 Ascend C API Reference / data movement

## Function
`DataCopy(dst, src, count)`: moves `count` elements between Global Memory and Local Memory (or Local<->Local).
It is the CopyIn/CopyOut foundation of every vector/scalar kernel (used by every example in this project).

## Function prototype (common count form)
```cpp
template <typename T>
__aicore__ inline void DataCopy(const LocalTensor<T>& dstLocal,
                                const GlobalTensor<T>& srcGlobal, const uint32_t count);
// Reverse: DataCopy(GlobalTensor dst, LocalTensor src, count); also Local<->Local
```
- The start address must be 32-byte aligned; `count` must satisfy the alignment granularity of the dtype.

## Simplest example design
- x is filled entirely with `7.0`. The kernel performs no computation, only `DataCopy`: GM->Local->Local->GM.
- Expect `z == x == 7.0`; the host verifies element by element.
- See `kernel.cpp` / `main.cpp` in the same directory; for the result see `RESULT.md`.
