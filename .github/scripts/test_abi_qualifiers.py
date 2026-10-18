#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(sys.platform.startswith("linux") and all(
    shutil.which(tool) for tool in ("cc", "abidw", "abidiff")), "Requires ELF tools")
class QualifierTests(unittest.TestCase):
    def test_read_only_handle_does_not_change_binary_calling_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name, qualifier, context in (("old", "", "WasmEdge_VMContext"),
                                             ("readonly", "const ", "WasmEdge_VMContext"),
                                             ("wrong", "const ", "WasmEdge_ConfigureContext")):
                source = root / f"{name}.c"
                source.write_text(f"struct {context} {{ int value; }};\n"
                                  f"int api({qualifier}struct {context} *p) {{ return p->value; }}\n")
                subprocess.run(["cc", "-g", "-O0", "-shared", "-fPIC", str(source),
                                "-Wl,-soname,libtest.so.0", "-o", str(root / f"{name}.so")], check=True)
            for name, expected in (("readonly", 0), ("wrong", 1)):
                with self.subTest(name=name):
                    result = subprocess.run([
                        sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                        "--types-only", str(root / "old.so"), str(root / f"{name}.so"),
                    ], capture_output=True, text=True)
                    self.assertEqual(result.returncode, expected, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
