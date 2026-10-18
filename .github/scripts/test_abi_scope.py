#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

from check_abi import require_function_scope


class ScopeTests(unittest.TestCase):
    def test_only_defined_visible_interfaces_are_rejected(self):
        require_function_scope("""
  1: 0001 1 FUNC GLOBAL DEFAULT 12 api@@V1
  2: 0000 0 OBJECT GLOBAL DEFAULT ABS V1
  3: 0000 0 OBJECT GLOBAL DEFAULT UND imported
  4: 0001 8 OBJECT LOCAL DEFAULT 12 local
  5: 0001 8 OBJECT GLOBAL HIDDEN 12 hidden
""")
        for kind in ("OBJECT", "TLS", "IFUNC", "NOTYPE"):
            with self.subTest(kind=kind), self.assertRaisesRegex(ValueError, "Outside public-function ABI scope"):
                require_function_scope(f"1: 0001 8 {kind} GLOBAL DEFAULT 12 exported")

    @unittest.skipUnless(sys.platform.startswith("linux") and all(
        shutil.which(tool) for tool in ("cc", "readelf", "abidw", "abidiff")), "Requires ELF tools")
    def test_exported_data_cannot_silently_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "api.c"
            source.write_text("struct WasmEdge_VMContext {int x;}; struct WasmEdge_VMContext *global; int api(void) {return 1;}\n")
            library = root / "api.so"
            subprocess.run(["cc", "-g", "-shared", "-fPIC", str(source),
                            "-Wl,-soname,libtest.so.0", "-o", str(library)], check=True)
            result = subprocess.run([
                sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                "--types-only", str(library), str(library),
            ], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("Outside public-function ABI scope", result.stderr)
            self.assertIn("global", result.stderr)


if __name__ == "__main__":
    unittest.main()
