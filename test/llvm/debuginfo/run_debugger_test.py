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

SKIP = 77
LIB_SUFFIX = {"darwin": ".dylib", "win32": ".dll"}.get(sys.platform, ".so")

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
        ("BT2", r"#\d+\s+.*\bfact\b.*\bN=1\b.*dwarf_basic\.c:\d+"),
        ("FACT_N", r"^\$\d+ = 1$"),
    ],
    "lldb": [
        ("BT1", r"frame #\d+: .*\bsumList\b.*dwarf_basic\.c:\d+"),
        ("BT1", r"frame #\d+: .*\brun\b.*dwarf_basic\.c:\d+"),
        ("LOCALS", r"^\(int\) Total = 0$"),
        ("HEAD_RAW", r"^\s+__addr = \d+$"),
        ("HEAD", r"Head = \(wasm\) 0x[0-9a-f]+ \{\n\s+\* = \{\n\s+Value = 1$"),
        ("WASM_VALUE", r"^N->Value = 1$"),
        ("COUNTER", r"^\(int\) Counter = 7$"),
        ("NODE1", r"^\(int\) Nodes\[1\]\.Value = 2$"),
        ("GLOBAL", r"^Counter = 7$"),
        ("BT2", r"frame #\d+: .*\bfact\b.*\bN=1\b.*dwarf_basic\.c:\d+"),
        ("FACT_N", r"^\(int\) N = 1$"),
    ],
}


# Debugger output that means the debugger cannot run in this environment.
UNUSABLE = {
    "gdb": [
        r"ptrace: Operation not permitted",
        r"Python scripting is not supported in this copy of GDB",
    ],
    "lldb": [
        r"there is no embedded script interpreter in this mode",
        r"process launch failed",
    ],
}


# At --optimize g, LLVM merges the `return 1` of fact into the shared exit
# block. The BREAK_FACT line then has no code, so the breakpoint moves to the
# next line and stops in the outermost fact frame (N=4) instead of N=1.
OG_CHECKS = {
    "gdb": {
        "BT2": r"#\d+\s+.*\bfact\b.*\bN=[14]\b.*dwarf_basic\.c:\d+",
        "FACT_N": r"^\$\d+ = [14]$",
    },
    "lldb": {
        "BT2": r"frame #\d+: .*\bfact\b.*\bN=[14]\b.*dwarf_basic\.c:\d+",
        "FACT_N": r"^\(int\) N = [14]$",
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
    """Return None if the debugger can stop a process, else the reason."""
    if debugger == "gdb":
        cmd = [exe, "-nx", "-batch",
               "-ex", "python print('@@PYTHON')",
               "-ex", "break main",
               "-ex", "run",
               "-ex", "python print('@@STOPPED', gdb.selected_frame().name())",
               "-ex", "kill",
               "--args", program, "--version"]
    else:
        cmd = [exe, "-b",
               "-o", "script print('@@PYTHON')",
               "-o", "breakpoint set --name main",
               "-o", "process launch",
               "-o", "script print('@@STOPPED', lldb.frame.GetFunctionName())",
               "-o", "process kill",
               "--", program, "--version"]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=120, env=env)
    except subprocess.TimeoutExpired:
        return "the probe did not finish"
    out = proc.stdout + proc.stderr
    if "@@PYTHON" not in out:
        return "no Python support"
    if not re.search(r"@@STOPPED \S*main", out):
        return "cannot stop a process at main"
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
    p.add_argument("--mode", choices=["aot", "jit"], required=True)
    p.add_argument("--wasmedge", required=True)
    p.add_argument("--wasmedgec", required=True)
    p.add_argument("--fixtures", required=True)
    p.add_argument("--helpers", required=True)
    p.add_argument("--opt", default="0")
    a = p.parse_args()

    exe = shutil.which(a.debugger)
    if not exe:
        print(f"{a.debugger} not found, skipping")
        return SKIP

    if a.debugger == "gdb":
        version = gdb_version(exe)
        if version is not None and version < 10:
            print(f"gdb {version} is older than 10, skipping")
            return SKIP
    env = dict(os.environ, DEBUGINFOD_URLS="")
    reason = probe(a.debugger, exe, a.wasmedge, env)
    if reason:
        print(f"{a.debugger} cannot run here ({reason}), skipping")
        return SKIP

    ext = a.debugger
    helper = os.path.join(a.helpers, f"wasmedge_{ext}.py")
    template = os.path.join(a.fixtures, f"basic.{ext}")
    src = os.path.join(a.fixtures, "dwarf_basic.c")
    work = tempfile.mkdtemp(prefix="wasmedge-dbg-")
    wasm = os.path.join(work, "dwarf_basic_O0.wasm")
    with open(os.path.join(a.fixtures, "dwarf_basic_O0.wasm.hex")) as f:
        with open(wasm, "wb") as out:
            out.write(bytes.fromhex("".join(f.read().split())))
    if a.mode == "aot":
        target = os.path.join(work, "dwarf_basic" + LIB_SUFFIX)
        subprocess.run([a.wasmedgec, "--debug-info", "--optimize", a.opt,
                        wasm, target], check=True)
        run_args = ["--run-mode=aot", "--reactor", target, "run"]
    else:
        run_args = ["--run-mode=jit", "--debug-info", "--reactor", wasm,
                    "run"]

    with open(template, encoding="utf-8") as f:
        script = f.read()
    script = (script.replace("@HELPER@", helper.replace(os.sep, "/"))
              .replace("@BREAK_SUMLIST@", str(marker(src, "BREAK_SUMLIST")))
              .replace("@BREAK_FACT@", str(marker(src, "BREAK_FACT"))))
    script_path = os.path.join(work, f"basic.{ext}")
    with open(script_path, "w", encoding="utf-8") as f:
        f.write(script)

    if a.debugger == "gdb":
        cmd = [exe, "-nx", "-batch", "-x", script_path, "--args",
               a.wasmedge] + run_args
    else:
        cmd = [exe, "-b", "-s", script_path, "--", a.wasmedge] + run_args
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=300, env=env)
    except subprocess.TimeoutExpired as e:
        print(f"{a.debugger} did not finish in {e.timeout} seconds")
        print(f"work directory kept: {work}")
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
    found = sections(out)
    missing = [(name, pattern) for name, pattern in checks
               if not re.search(pattern, found.get(name, ""), re.M)]
    if missing:
        log = out + proc.stderr
        for pattern in UNUSABLE[a.debugger]:
            if re.search(pattern, log):
                print(f"{a.debugger} cannot run here ({pattern}), skipping")
                shutil.rmtree(work, ignore_errors=True)
                return SKIP
        for name, pattern in missing:
            print(f"missing in @@{name}: {pattern}")
        print(f"work directory kept: {work}")
        return 1
    shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
