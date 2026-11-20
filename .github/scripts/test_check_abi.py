#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import contextlib
import io
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

from check_abi import compare, elf_identity, soname, symbols


class SymbolTests(unittest.TestCase):
    def test_versioned_exports(self):
        output = """
  1: 00000010 10 FUNC GLOBAL DEFAULT 12 WasmEdge_Add@@WASMEDGE_0.18
  2: 00000020 10 FUNC GLOBAL DEFAULT 12 WasmEdge_Add@WASMEDGE_0.16
  3: 00000000  0 FUNC GLOBAL DEFAULT UND WasmEdge_Import
  4: 00000030 10 FUNC LOCAL DEFAULT 12 WasmEdge_Internal
  5: 00000040 10 FUNC GLOBAL HIDDEN 12 WasmEdge_Hidden
  6: 00000050 10 FUNC GLOBAL DEFAULT 12 unrelated
"""
        self.assertEqual(symbols(output), {
            ("WasmEdge_Add@WASMEDGE_0.18", "FUNC"),
            ("WasmEdge_Add@WASMEDGE_0.16", "FUNC"),
            ("unrelated", "FUNC"),
        })

    def test_symbol_kind_change(self):
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(compare("so.0", "so.0", {("api", "FUNC")},
                                     {("api", "OBJECT")}), 1)

    def test_elf_identity(self):
        header = "Class: ELF64\nData: little endian\nType: DYN (Shared object file)\nMachine: X86-64\n"
        self.assertEqual(elf_identity(header)[0], "ELF64")
        with self.assertRaises(ValueError):
            elf_identity(header.replace("DYN", "EXEC"))
        with self.assertRaises(ValueError):
            elf_identity("")

    def test_invalid_input(self):
        with self.assertRaises(ValueError):
            symbols("")
        with self.assertRaises(ValueError):
            soname("")

    def test_soname(self):
        self.assertEqual(
            soname("0x0 (SONAME) Library soname: [libwasmedge.so.0]"),
            "libwasmedge.so.0",
        )

    def test_comparison(self):
        old = {"WasmEdge_Add@WASMEDGE_0.16"}
        new = {"WasmEdge_Add@WASMEDGE_0.17"}
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(compare("so.0", "so.0", old, new), 1)
            self.assertEqual(compare("so.0", "so.0", old, old | new), 0)
            self.assertEqual(compare("so.0", "so.1", old, new), 2)

    @unittest.skipUnless(
        sys.platform.startswith("linux") and shutil.which("cc") and shutil.which("readelf"),
        "ELF integration test requires Linux, cc and readelf",
    )
    def test_real_elf_libraries(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "api.c"
            source.write_text("void WasmEdge_Add(void) {}\n")
            libraries = []
            for version in ("0.16", "0.17"):
                script = root / f"{version}.map"
                script.write_text(f"WASMEDGE_{version} {{ global: WasmEdge_Add; local: *; }};\n")
                library = root / f"{version}.so"
                subprocess.run([
                    "cc", "-shared", "-fPIC", str(source),
                    "-Wl,-soname,libwasmedge.so.0",
                    f"-Wl,--version-script={script}", "-o", str(library),
                ], check=True)
                libraries.append(str(library))
            checker = pathlib.Path(__file__).with_name("check_abi.py")
            for candidate, expected in ((libraries[0], 0), (libraries[1], 1)):
                result = subprocess.run(
                    [sys.executable, str(checker), libraries[0], candidate],
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, expected, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
