#!/usr/bin/env python3
"""
gen.py — render simulatable AscendC example units from manifest.yaml.

For each API with arity in {binary, unary, scalar}, generate
  examples/<lib>/<category>/<name_lower>/{kernel.cpp, main.cpp, CMakeLists.txt, meta.json}
Other arities (manual/reduce/compare/...) are not auto-generated yet (hand-written or future templates).

Usage:
  python3 harness/gen.py                # generate all auto-generatable units
  python3 harness/gen.py Add Exp Sub    # generate only the given APIs (case-insensitive)
"""
import json
import os
import sys

import yaml

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TPL = os.path.join(ROOT, "harness", "templates")
EXAMPLES = os.path.join(ROOT, "examples")
MANIFEST = os.path.join(ROOT, "manifest.yaml")

# arity -> (kernel template, host template, number of inputs, header name)
ARITY_MAP = {
    "binary": ("binary_kernel.cpp.in", "host_2in.cpp.in", 2, "kernel_operator_vec_binary_intf.h"),
    "unary":  ("unary_kernel.cpp.in",  "host_1in.cpp.in", 1, "kernel_operator_vec_unary_intf.h"),
    "scalar": ("scalar_kernel.cpp.in", "host_1in.cpp.in", 1, "kernel_operator_vec_binary_scalar_intf.h"),
    "cast":   ("cast_kernel.cpp.in",   "host_cast.cpp.in", 1, "kernel_operator_vec_vconv_intf.h"),
    "reduce": ("reduce_kernel.cpp.in", "host_reduce.cpp.in", 1, "kernel_operator_vec_reduce_intf.h"),
    # activation: high-level adv_api unary, simple count mode; the prototype header is specified by the manifest's proto_header
    "activation": ("activation_kernel.cpp.in", "host_1in.cpp.in", 1, None),
    # binary_act: high-level adv_api binary (Power/Fmod/Hypot...), binary template + include
    "binary_act": ("binary_act_kernel.cpp.in", "host_2in.cpp.in", 2, None),
    # integer bitwise ops: reuse the binary/unary kernel templates (kdtype=int16_t), with the integer host
    "bitbin": ("binary_kernel.cpp.in", "host_2in_int.cpp.in", 2, "kernel_operator_vec_binary_intf.h"),
    "bitun":  ("unary_kernel.cpp.in",  "host_1in_int.cpp.in", 1, "kernel_operator_vec_unary_intf.h"),
}
# kernel dtypes supported by auto-generation (used for the skip check: generate as long as the manifest dtypes contain one of them)
GENERATABLE_DTYPES = {"float", "int16_t", "int32_t"}

# AscendC kernel interface header directory (authoritative prototype source); prefer the toolkit environment variable
_TK = os.environ.get(
    "ASCEND_TOOLKIT_HOME",
    os.path.expanduser("~/miniforge3/envs/cannsim/cann/cann-9.1.0-beta.1"))
HDR_DIR = os.path.join(_TK, "aarch64-linux", "asc", "include", "interface")


def find_header(header):
    """Locate the header by file name in the interface and adv_api subtrees."""
    for base in (HDR_DIR, os.path.join(os.path.dirname(HDR_DIR), "adv_api")):
        # direct path
        p = os.path.join(HDR_DIR, header)
        if os.path.exists(p):
            return p
    # search recursively under adv_api
    advroot = os.path.join(os.path.dirname(HDR_DIR), "adv_api")
    for dp, _, fs in os.walk(advroot):
        if os.path.basename(header) in fs:
            return os.path.join(dp, os.path.basename(header))
    return None


def extract_prototype(op, header):
    """Extract the count-mode prototype of op from the interface header.

    A declaration may span multiple lines and end with `;` (declaration) or `{` (adv_api inline definition).
    Parameter types vary (LocalTensor<T> / template U&/S&/V& / uint32_t); do not filter by type.
    Prefer the "simple count mode": ends with count/dataSize/calCount) and contains no mask/repeatParams/sharedTmpBuffer.
    """
    path = find_header(header)
    if not path:
        return None
    import re as _re
    text = open(path).read()
    # capture `void OP( ... )` followed by ; or { (possibly multi-line)
    pat = _re.compile(r"void\s+%s\s*\((?:[^;{])*?\)\s*[;{]" % _re.escape(op), _re.S)
    cands = [_re.sub(r"\s+", " ", m.group(0)).strip().rstrip(";{ ") for m in pat.finditer(text)]
    if not cands:
        return None
    simple = [c for c in cands
              if _re.search(r"(count|dataSize|calCount)\s*\)$", c)
              and not _re.search(r"mask|repeatParams|sharedTmpBuffer|Tiling", c)]
    if simple:
        return min(simple, key=len)
    return min(cands, key=len)


def read_tpl(name):
    with open(os.path.join(TPL, name)) as f:
        return f.read()


def render(text, subs):
    for k, v in subs.items():
        text = text.replace("@%s@" % k, str(v))
    return text


def gen_one(api, defaults):
    name = api["name"]
    arity = api.get("arity")
    if arity not in ARITY_MAP:
        return None  # manual / unsupported arity, skip
    kdtype = api.get("kdtype", "float")  # kernel/host data type
    if kdtype not in GENERATABLE_DTYPES:
        print("  [skip] %s: unsupported kdtype %s" % (name, kdtype))
        return None

    kern_tpl, host_tpl, ninp, header = ARITY_MAP[arity]
    header = api.get("proto_header", header)  # activation etc. specify the prototype header via the manifest
    lib = api["lib"]
    category = api["category"]
    soc_build = defaults["soc_build"]
    soc_run = defaults["soc_run"]
    inputs = api.get("inputs", {})
    expect = api.get("expect", 0.0)
    tag = name.upper()

    subs = {
        "PROJECT": "ascendc_%s_sim" % name.lower(),
        "SOC_BUILD": soc_build,
        "OP": name,
        "DTYPE": kdtype,
        "TAG": tag,
        "EXPECT": expect,
        "IN_X": inputs.get("x", 0.0),
        "IN_Y": inputs.get("y", 0.0),
        "SCALAR": inputs.get("s", 0.0),
        "SRC_DTYPE": api.get("src_dtype", "float"),
        "DST_DTYPE": api.get("dst_dtype", "int32_t"),
        "ROUND": api.get("round", "CAST_RINT"),
        "TOL": api.get("tol", "1e-3"),
        "INCLUDE": api.get("include", ""),
        "EXTRA_ARGS": api.get("extra_args", ""),
    }

    outdir = os.path.join(EXAMPLES, lib, category, name.lower())
    os.makedirs(outdir, exist_ok=True)

    with open(os.path.join(outdir, "kernel.cpp"), "w") as f:
        f.write(render(read_tpl(kern_tpl), subs))
    with open(os.path.join(outdir, "main.cpp"), "w") as f:
        f.write(render(read_tpl(host_tpl), subs))
    with open(os.path.join(outdir, "CMakeLists.txt"), "w") as f:
        f.write(render(read_tpl("CMakeLists.txt.in"), subs))

    disp_dtype = ("%s->%s" % (subs["SRC_DTYPE"], subs["DST_DTYPE"])) if arity == "cast" else kdtype
    meta = {
        "name": name, "lib": lib, "category": category, "arity": arity,
        "dtype": disp_dtype, "soc_build": soc_build, "soc_run": soc_run,
        "tag": tag, "inputs": inputs, "expect": expect,
        "doc_url": api.get("doc_url", ""),
        "pass_marker": "%s SIMULATION PASSED" % tag,
        "dir": os.path.relpath(outdir, ROOT),
    }
    with open(os.path.join(outdir, "meta.json"), "w") as f:
        json.dump(meta, f, ensure_ascii=False, indent=2)

    # doc.md: functional description + real prototype extracted from the header + example design
    proto = extract_prototype(name, header) or "(could not be extracted from the header, see the source link)"
    desc = api.get("desc", "AscendC vector compute interface `%s`, computes element-wise over a LocalTensor." % name)
    arity_note = {"binary": "binary, `dst = OP(src0, src1)` element-wise",
                  "unary": "unary, `dst = OP(src)` element-wise",
                  "scalar": "vector-scalar, `dst = OP(src, scalar)` element-wise",
                  "cast": "cast, `dst<%s> = OP(src<%s>, roundMode)` element-wise"
                          % (subs["DST_DTYPE"], subs["SRC_DTYPE"]),
                  "reduce": "reduction, `dst[0] = OP(src[0..count], tmpBuffer)` (256 elements -> scalar)",
                  "activation": "high-level activation, `dst = OP(src)` element-wise (adv_api, simple count mode)",
                  "bitbin": "integer bitwise op (binary), `dst = OP(src0, src1)` element-wise",
                  "bitun": "integer bitwise op (unary), `dst = OP(src)` element-wise",
                  "binary_act": "high-level binary (adv_api), `dst = OP(src0, src1)` element-wise"}.get(arity, arity)
    if arity == "scalar":
        in_desc = "src filled entirely with `%s`, scalar `%s`" % (inputs.get("x"), inputs.get("s"))
    elif arity in ("binary", "bitbin", "binary_act"):
        in_desc = "src0=`%s`, src1=`%s`" % (inputs.get("x"), inputs.get("y"))
    elif arity == "cast":
        in_desc = "src(%s) filled entirely with `%s`, RoundMode=`%s`" % (
            subs["SRC_DTYPE"], inputs.get("x"), subs["ROUND"])
    else:
        in_desc = "src filled entirely with `%s`" % inputs.get("x")
    doc_lines = [
        "# Ascend C / %s" % name,
        "",
        "- Category: vector compute / %s (%s)" % (category, arity_note),
        "- dtype: %s (this example); the header also supports %s" % (disp_dtype, ", ".join(api.get("dtypes", []))),
        "- Source: <%s>" % api["doc_url"] if api.get("doc_url") else "- Source: see the CANN 9.1.0 Ascend C API reference",
        "",
        "## Functionality",
        desc,
        "",
        "## Function prototype (count mode, taken from toolkit header `%s`)" % header,
        "```cpp",
        proto,
        "```",
        "",
        "## Minimal example design",
        "- %s -> expect `dst %s %s`, host-side element-wise verify%s." % (
            in_desc,
            "==" if arity in ("cast", "bitbin", "bitun") else "~", expect,
            " (exact match)" if arity in ("cast", "bitbin", "bitun") else " (tol %s)" % subs["TOL"]),
        "- Total length 8*2048, 8 cores, double buffer; SOC build `%s`, simulation `%s`." % (soc_build, soc_run),
        "- See `kernel.cpp` / `main.cpp` in the same directory; run results in `RESULT.md`.",
        "",
    ]
    with open(os.path.join(outdir, "doc.md"), "w") as f:
        f.write("\n".join(doc_lines))

    print("  [gen] %-10s -> %s" % (name, meta["dir"]))
    return meta


def main():
    with open(MANIFEST) as f:
        m = yaml.safe_load(f)
    defaults = m["defaults"]
    want = set(a.lower() for a in sys.argv[1:])

    n = 0
    for api in m["apis"]:
        if want and api["name"].lower() not in want:
            continue
        if gen_one(api, defaults):
            n += 1
    print("Generated %d units." % n)


if __name__ == "__main__":
    main()
