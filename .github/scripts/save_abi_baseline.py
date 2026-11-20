#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Save the current build's ABI data without requiring a previous baseline."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile

from check_abi import elf_identity, readelf, soname, symbols


def compiler_metadata(cache):
    values = {}
    for line in cache.read_text().splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line:
            key, value = line.split("=", 1)
            values[key.split(":", 1)[0]] = value
    compiler = values.get("CMAKE_CXX_COMPILER")
    if not compiler:
        raise ValueError("CMake cache does not identify the C++ compiler")
    return {
        "path": compiler,
        "version": subprocess.check_output([compiler, "--version"], text=True),
        "build_type": values.get("CMAKE_BUILD_TYPE", ""),
        "flags": {key: value for key, value in values.items()
                  if key.startswith("CMAKE_CXX_FLAGS")},
    }


def save(library, version, output, cache):
    compiler = compiler_metadata(cache)
    if ".debug_info" not in readelf(library, "--sections"):
        raise ValueError("Release library lacks embedded DWARF")
    subprocess.run(["abidiff", str(library), str(library)], check=True)
    output.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256(library.read_bytes()).hexdigest()
    manifest = {
        "schema": 1,
        "version": version,
        "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
        "identity": elf_identity(readelf(library, "--file-header")),
        "soname": soname(readelf(library, "--dynamic")),
        "symbols": sorted(symbols(readelf(library, "--dyn-syms"))),
        "sha256": digest,
        "compiler": compiler,
        "abidiff": subprocess.check_output(["abidiff", "--version"], text=True),
    }
    (output / "WasmEdge-abi-linux-x86_64.json").write_text(json.dumps(manifest, indent=2) + "\n")
    with tarfile.open(output / "WasmEdge-abi-linux-x86_64.tar.gz", "w:gz") as archive:
        archive.add(library, arcname="libwasmedge.so.debug", recursive=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("library", type=Path)
    parser.add_argument("version")
    parser.add_argument("--output", type=Path, default=Path("abi-baseline"))
    parser.add_argument("--cmake-cache", type=Path, default=Path("build/CMakeCache.txt"))
    args = parser.parse_args()
    save(args.library.resolve(strict=True), args.version, args.output, args.cmake_cache)


if __name__ == "__main__":
    main()
