#!/usr/bin/env python3
"""
fix_marker_order.py — move the "<TAG> SIMULATION PASSED/FAILED" marker so it is
printed BEFORE the ACL teardown (aclrtResetDevice/aclFinalize) instead of after.

On some hosts (observed on x86_64 with CANN 9.1.0-beta.1) aclFinalize() ends the
process / closes the simulator's stdout capture, so a marker printed afterwards is
never written to record.log and the unit is wrongly reported as sim_failed even
though the result is correct. The `errors` count is already final before teardown,
so the marker can be emitted earlier with no change in meaning. Idempotent.

Targets: harness/templates/host_*.cpp.in and every examples/**/main.cpp.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A run of CHECK_ACL(...) cleanup lines, immediately followed by the marker block:
#   if (errors == 0) { printf("<TAG> SIMULATION PASSED\n"); return 0; }
#   printf("<TAG> SIMULATION FAILED (%d errors)\n", errors); return 1;
PAT = re.compile(
    r"(?P<cleanup>(?:[ \t]*CHECK_ACL\(.*\);[ \t]*\n)+)"
    r"[ \t]*\n?"
    r"[ \t]*if \(errors == 0\) \{\s*"
    r"printf\(\"(?P<tag>[^\"]+?) SIMULATION PASSED\\n\"\);\s*"
    r"return 0;\s*"
    r"\}\s*"
    r"printf\(\"[^\"]+? SIMULATION FAILED \(%d errors\)\\n\", errors\);\s*"
    r"return 1;",
    re.MULTILINE,
)


def transform(text):
    def repl(m):
        cleanup = m.group("cleanup").rstrip("\n")
        tag = m.group("tag")
        return (
            "    // Emit the PASS/FAIL marker BEFORE ACL teardown: on some hosts aclFinalize()\n"
            "    // ends the process / closes the simulator's stdout capture, so a marker printed\n"
            "    // afterwards is never recorded. errors is already final here.\n"
            "    if (errors == 0) printf(\"%s SIMULATION PASSED\\n\");\n"
            "    else             printf(\"%s SIMULATION FAILED (%%d errors)\\n\", errors);\n"
            "    fflush(stdout);\n\n"
            "%s\n\n"
            "    return errors == 0 ? 0 : 1;" % (tag, tag, cleanup)
        )

    new, n = PAT.subn(repl, text)
    return new, n


def main():
    files = []
    tdir = os.path.join(ROOT, "harness", "templates")
    for f in sorted(os.listdir(tdir)):
        if f.startswith("host_") and f.endswith(".cpp.in"):
            files.append(os.path.join(tdir, f))
    for dp, _, fs in os.walk(os.path.join(ROOT, "examples")):
        if "main.cpp" in fs:
            files.append(os.path.join(dp, "main.cpp"))

    changed, already, has_marker_unmatched = [], [], []
    for path in files:
        txt = open(path).read()
        has_marker = "SIMULATION PASSED" in txt
        # already fixed if the marker now precedes the teardown
        if "Emit the PASS/FAIL marker BEFORE ACL teardown" in txt:
            already.append(path)
            continue
        new, n = transform(txt)
        if n > 0:
            open(path, "w").write(new)
            changed.append(path)
        elif has_marker:
            has_marker_unmatched.append(path)

    rel = lambda p: os.path.relpath(p, ROOT)
    print("changed (%d):" % len(changed))
    for p in changed:
        print("  +", rel(p))
    print("already fixed (%d)" % len(already))
    if has_marker_unmatched:
        print("HAS MARKER BUT PATTERN NOT MATCHED (%d) -- inspect manually:" % len(has_marker_unmatched))
        for p in has_marker_unmatched:
            print("  ?", rel(p))


if __name__ == "__main__":
    main()
