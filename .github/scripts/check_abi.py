#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Check Linux shared libraries for removed ABI exports and type changes."""

import argparse
import os
import re
import subprocess
import sys


def readelf(library, option):
    return subprocess.check_output(
        ["readelf", "--wide", option, str(library)],
        text=True,
        env={**os.environ, "LC_ALL": "C"},
    )


def soname(output):
    match = re.search(r"\(SONAME\).*\[([^\]]+)\]", output)
    if not match:
        raise ValueError("Library has no DT_SONAME")
    return match.group(1)


def elf_identity(output):
    identity = []
    for field in ("Class", "Data", "Type", "Machine"):
        match = re.search(rf"^\s*{field}:\s*(.+)$", output, re.MULTILINE)
        if not match:
            raise ValueError(f"Missing ELF header field: {field}")
        identity.append(match.group(1).strip())
    if not identity[2].startswith("DYN "):
        raise ValueError("Expected an ELF shared object")
    return tuple(identity)


def symbols(output):
    exports = set()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 8 or not fields[0].endswith(":"):
            continue
        if fields[4] not in {"GLOBAL", "WEAK"}:
            continue
        if fields[5] not in {"DEFAULT", "PROTECTED"} or fields[6] == "UND":
            continue
        name = fields[7]
        if fields[3] in {"FUNC", "OBJECT", "TLS", "IFUNC"}:
            exports.add((name.replace("@@", "@"), fields[3]))
    if not exports:
        raise ValueError("Library has no exported ABI symbols")
    return exports


def compare(old_soname, new_soname, old_symbols, new_symbols):
    if old_soname != new_soname:
        print(f"SONAME changed: {old_soname} -> {new_soname}; ABI transition required")
        return 2
    missing = old_symbols - new_symbols
    if missing:
        print(f"Removed ABI symbols under unchanged SONAME {old_soname}:")
        for symbol in sorted(missing):
            print(f"  {symbol}")
        return 1
    print(f"Preserved {len(old_symbols)} ABI symbols under {old_soname}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline")
    parser.add_argument("candidate")
    parser.add_argument("--types", action="store_true", help="Require DWARF and run abidiff")
    args = parser.parse_args()
    try:
        if elf_identity(readelf(args.baseline, "--file-header")) != elf_identity(
            readelf(args.candidate, "--file-header")
        ):
            raise ValueError("ELF class, byte order or machine differs between libraries")
        result = compare(
            soname(readelf(args.baseline, "--dynamic")),
            soname(readelf(args.candidate, "--dynamic")),
            symbols(readelf(args.baseline, "--dyn-syms")),
            symbols(readelf(args.candidate, "--dyn-syms")),
        )
        if result == 2 or not args.types:
            return result
        for library in (args.baseline, args.candidate):
            if not re.search(r"\.(?:debug_info|zdebug_info)\s", readelf(library, "--sections")):
                raise ValueError(f"Missing embedded DWARF type information: {library}")
        status = subprocess.run(
            ["abidiff", "--no-added-syms", args.baseline, args.candidate], check=False
        ).returncode
        if status:
            print(f"abidiff reported changes or an analysis failure (status {status})")
            return 1
        return result
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"ABI check failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
