#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

import hex_tool

SKIP = 77
# A comma-separated list of debuggers whose tests must run. A test of a
# listed debugger fails where it would skip.
REQUIRED = os.environ.get("WASMEDGE_REQUIRE_DEBUGGER", "").split(",")
# The wasmedge tool defines wmain on Windows.
ENTRY = "wmain" if os.name == "nt" else "main"

# Each check is (section marker printed by the debugger script, regex that
# must match inside that section).
CHECKS = {
    "gdb": [
        ("BT1", r"#\d+\s+.*\bsumList\b.*dwarf_basic\.c:\d+"),
        ("BT1", r"#\d+\s+.*\brun\b.*dwarf_basic\.c:\d+"),
        ("LOCALS", r"^Total = 0$"),
        ("HEAD_RAW", r"__addr = \d+"),
        ("HEAD", r"\(wasm\) 0x[0-9a-f]+ -> \{Value = 1,"),
        ("WASM_VALUE", r"^\$\d+ = 1$"),
        ("COUNTER", r"^\$\d+ = 7$"),
        ("NODE1", r"^\$\d+ = 2$"),
        ("GLOBAL", r"^\$\d+ = 7$"),
        ("CONSTANT", r"^\$\d+ = 3$"),
        ("NULL", r"^\$\d+ = \(wasm\) 0x0$"),
        ("NULL_HOST", r"^\$\d+ = \(struct Node \*\) 0x0( <\w+>)?$"),
        ("UNREADABLE", r"^\(wasm\) <unavailable>$"),
        ("BT2", r"#\d+\s+.*\bfact\b.*\bN=1\b.*dwarf_basic\.c:\d+"),
        ("FACT_N", r"^\$\d+ = 1$"),
    ],
    "lldb": [],
}


# On a big-endian target, the compiler emits only functions and lines, so
# only the backtrace of the first stop is checked. The debugger script then
# fails on the variables, so its exit status is not checked either.
LINES_ONLY = sys.byteorder == "big"
LINES_ONLY_SECTIONS = {"BT1"}


# At --optimize g, LLVM merges the `return 1` of fact into the shared exit
# block. The BREAK_FACT line then has no code, so the breakpoint moves to the
# next line and stops in the outermost fact frame (N=4) instead of N=1.
OG_CHECKS = {
    "gdb": {
        "BT2": r"#\d+\s+.*\bfact\b.*\bN=[14]\b.*dwarf_basic\.c:\d+",
        "FACT_N": r"^\$\d+ = [14]$",
    },
}


def gdb_version(exe):
    try:
        out = subprocess.run([exe, "--version"], capture_output=True,
                             text=True, timeout=60).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    m = re.search(r"(\d+)\.\d+", out.splitlines()[0] if out else "")
    return int(m.group(1)) if m else None


def probe(debugger, exe, program, env):
    """Check that the debugger can stop a process.

    Return None if it can. Return the reason and whether the test fails or
    is skipped if it cannot. For gdb, a breakpoint on main that does not
    resolve is a failure, because the tool has that symbol. lldb stops at
    the entry address of the executable, so the probe needs no symbol of
    the tool, which a release build on Windows can lack.
    """
    if debugger == "gdb":
        cmd = [exe, "-nx", "-batch",
               "-ex", "python print('@@PYTHON')",
               "-ex", f"break {ENTRY}",
               "-ex", "python print('@@RESOLVED', "
                      "not gdb.breakpoints()[0].pending)",
               "-ex", "run",
               "-ex", "python print('@@STOPPED', gdb.selected_frame().name())",
               "-ex", "kill",
               "--args", program, "--version"]
    else:
        cmd = [exe, "-b", "--no-lldbinit",
               "-o", "script print('@@PYTHON')",
               "-o", "process launch --stop-at-entry",
               "-o", "script print('@@STOPPED', lldb.process.GetState() == "
                     "lldb.eStateStopped)",
               "-o", "process kill",
               "--", program, "--version"]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=120, env=env)
    except subprocess.TimeoutExpired:
        return "the probe did not finish", False
    out = proc.stdout + proc.stderr
    if "@@PYTHON" not in out:
        return "no Python support", False
    if debugger == "gdb":
        if "@@RESOLVED True" not in out:
            return f"the breakpoint on {ENTRY} does not resolve", True
        if not re.search(rf"@@STOPPED \S*{ENTRY}", out):
            return f"cannot stop a process at {ENTRY}", False
    elif "@@STOPPED True" not in out:
        return "cannot stop a process at its entry", False
    return None


def marker(src, name):
    with open(src, encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            if name in line:
                return n
    raise SystemExit(f"marker {name} not found")


def sections(out):
    result = {}
    current = None
    for line in out.splitlines():
        m = re.match(r"@@(\w+)$", line)
        if m:
            current = m.group(1)
            result[current] = []
        elif current:
            result[current].append(line)
    return {k: "\n".join(v) for k, v in result.items()}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--debugger", choices=["gdb", "lldb"], required=True)
    p.add_argument("--mode", choices=["aot", "jit", "lazyjit"],
                   required=True)
    p.add_argument("--wasmedge", required=True)
    p.add_argument("--wasmedgec", required=True)
    p.add_argument("--fixtures", required=True)
    p.add_argument("--helpers", required=True)
    p.add_argument("--opt", default="0")
    p.add_argument("--keep-work", action="store_true",
                   help="keep the work directory and print its path")
    a = p.parse_args()
    required = a.debugger in REQUIRED
    skip, action = (1, "failing") if required else (SKIP, "skipping")

    exe = shutil.which(a.debugger)
    if not exe:
        print(f"{a.debugger} not found, {action}")
        return skip

    if a.debugger == "gdb":
        version = gdb_version(exe)
        if version is not None and version < 10:
            print(f"gdb {version} is older than 10, {action}")
            return skip
    env = dict(os.environ, DEBUGINFOD_URLS="")
    result = probe(a.debugger, exe, a.wasmedge, env)
    if result:
        reason, fatal = result
        if fatal:
            print(f"{a.debugger} probe failed: {reason}")
            return 1
        print(f"{a.debugger} cannot run here ({reason}), {action}")
        return skip

    work = tempfile.mkdtemp(prefix="wasmedge-dbg-")
    try:
        return run(a, exe, env, work)
    finally:
        if a.keep_work:
            print(f"work directory kept: {work}")
        else:
            shutil.rmtree(work, ignore_errors=True)


def run(a, exe, env, work):
    ext = a.debugger
    helper = os.path.join(a.helpers, f"wasmedge_{ext}.py")
    template = os.path.join(a.fixtures, f"basic.{ext}")
    if a.debugger == "lldb" and not (os.path.exists(template) and
                                     os.path.exists(helper)):
        print(f"{a.debugger} test files not available, skipping")
        return SKIP

    src = os.path.join(a.fixtures, "dwarf_basic.c")
    wasm = os.path.join(work, "dwarf_basic_O0.wasm")
    hex_tool.from_hex(os.path.join(a.fixtures, "dwarf_basic_O0.wasm.hex"),
                      wasm)
    if a.mode == "aot":
        target = os.path.join(work, "dwarf_basic.so")
        subprocess.run([a.wasmedgec, "--debug-info", "--optimize", a.opt,
                        wasm, target], check=True)
        run_args = ["--run-mode=aot", "--reactor", target, "run"]
    else:
        run_args = [f"--run-mode={a.mode}", "--debug-info", "--reactor",
                    wasm, "run"]

    with open(template, encoding="utf-8") as f:
        script = f.read()
    script = (script.replace("@HELPER@", helper)
              .replace("@BREAK_SUMLIST@", str(marker(src, "BREAK_SUMLIST")))
              .replace("@BREAK_FACT@", str(marker(src, "BREAK_FACT"))))
    script_path = os.path.join(work, f"basic.{ext}")
    with open(script_path, "w", encoding="utf-8") as f:
        f.write(script)

    if a.debugger == "gdb":
        cmd = [exe, "-nx", "-batch", "-x", script_path, "--args",
               a.wasmedge] + run_args
    else:
        cmd = [exe, "-b", "--no-lldbinit", "-s", script_path, "--",
               a.wasmedge] + run_args
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=300)
    except subprocess.TimeoutExpired as e:
        print(f"{a.debugger} did not finish in {e.timeout} seconds")
        return 1
    out = proc.stdout
    print(out)
    if proc.stderr:
        print(proc.stderr, file=sys.stderr)

    checks = CHECKS[a.debugger]
    if a.opt == "g":
        override = OG_CHECKS.get(a.debugger, {})
        checks = [(name, override.get(name, pattern))
                  for name, pattern in checks]
    if LINES_ONLY:
        checks = [(name, pattern) for name, pattern in checks
                  if name in LINES_ONLY_SECTIONS]
    found = sections(out)
    missing = [(name, pattern) for name, pattern in checks
               if not re.search(pattern, found.get(name, ""), re.M)]
    for name, pattern in missing:
        print(f"missing in @@{name}: {pattern}")
    failed = proc.returncode != 0 and not LINES_ONLY
    if proc.returncode != 0:
        print(f"{a.debugger} exited with status {proc.returncode}")
    return 1 if missing or failed else 0


if __name__ == "__main__":
    sys.exit(main())
