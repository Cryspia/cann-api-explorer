# Ascend C · std (SKIPPED — not an operator)

> 中文版: [doc.zh-CN.md](doc.zh-CN.md)

- Category: high-level adv_api / std
- Status: **skipped** — `adv_api/std/` is not a statistics (standard-deviation) operator.

## Why this is skipped
The directory `aarch64-linux/asc/include/adv_api/std/` does not contain a "Std" / standard-deviation
operator. It is the `AscendC::Std` C++-standard-library shim namespace used internally by other
high-level APIs. Its headers are:

- `algorithm.h` — `AscendC::Std::min` / `AscendC::Std::max`
- `cmath.h` — `AscendC::Std::sqrt` / `AscendC::Std::abs`
- `type_traits.h` — `enable_if`, `conditional`, `is_convertible`, `is_base_of`, ... (compile-time traits)
- `tuple.h` — `AscendC::Std::tuple` and helpers
- `utility.h` — `index_sequence` and related helpers

These are header-only template utilities (no kernel, no tensor I/O, no numeric golden output), so there
is no card-free-simulatable unit with a verifiable result to build here. A grep across `adv_api/` for an
`__aicore__ Std(...)` entry point or any "standard deviation" operator returns nothing on this toolkit
(CANN 9.1.0-beta.1, arch 3510).

The actual statistics / normalization high-level operators that *are* covered live in
`adv_api/normalization/` and neighbours: see the sibling units `normalize/` and `welfordfinalize/`
(and the existing `groupnorm/`, `layernorm/`, `batchnorm/`, `deepnorm/`, `rmsnorm/`).
