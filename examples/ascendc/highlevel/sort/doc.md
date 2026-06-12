# Ascend C · Sort (high-level, tiling-free)

- Category: high-level adv_api / sorting
- APIs covered: `AscendC::Sort`
- include: `lib/sort/sort.h`
- Source: CANN 9.1.0 Ascend C API Reference / Sort

## Function
`Sort<T, U>(dstValue, dstIndex, srcValue, srcIndex, sharedTmpBuffer, calCount)`:
sorts the first `calCount` elements by value, and carries each element's index along with it. 32 elements form one sort region.

## Key finding: this implementation is **ascending**
Measured in card-free (no-NPU) simulation (reversed input `val=[31..0], idx=[0..31]`):
```
dst[ 0] val=0  idx=31     # smallest value comes first
dst[31] val=31 idx=0      # largest value comes last
```
That is, `AscendC::Sort` outputs in **ascending** order by value, and `dstIndex[i]` is the position of that value in the original array.
> Pitfall record: at first the expectation was written following the convention that "hardware Sort defaults to descending", and an ascending input `[0..31]` was used.
> After ascending sort the output was unchanged -> misjudged as "not sorted / identity output". Switching to a **reversed input** was what distinguished the true direction.

## Minimal example design
- `N=32` float, input `val[i]=31-i` (reversed), `idx[i]=i`.
- Expect: `dst[i]=i` (ascending), `dstIdx[i]=31-i` (the original index of value i).
- Tiling-free: Sort's count mode needs no external tiling structure; `tmp` of 8192B is enough.
- Single core, host verify errors=0 (see `RESULT.md`, instr≈1774).

## Comparison with TopK
TopK needs host `TopKTilingFunc` to compute `TopkTiling` and pass it in via GM; Sort's count overload is completely tiling-free and lighter weight.
Both live under `lib/sort/`; compiling requires adding the include path `aarch64-linux/asc/include/adv_api/sort`.
