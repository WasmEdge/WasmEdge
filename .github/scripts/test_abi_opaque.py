#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(sys.platform.startswith("linux") and all(
    shutil.which(tool) for tool in ("cc", "readelf", "abidiff")), "Requires ELF tools")
class OpaqueTests(unittest.TestCase):
    def test_opaque_pointer_to_value_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for label, access in (("old", "*"), ("new", "")):
                source = root / f"{label}.c"
                source.write_text("struct WasmEdge_VMContext { int value; };\n"
                                  f"int api(struct WasmEdge_VMContext {access}value) {{ return sizeof(value); }}\n")
                subprocess.run(["cc", "-g", "-O0", "-shared", "-fPIC", str(source),
                                "-Wl,-soname,libtest.so.0", "-o", str(root / f"{label}.so")], check=True)
            result = subprocess.run([
                sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                "--types", str(root / "old.so"), str(root / "new.so"),
            ], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)

    def test_pointer_layout_only(self):
        for name in ("WasmEdge_VMContext", "WasmEdge_ConfigureContext", "WasmEdge_CompilerContext", "WasmEdge_Async", "wasm_engine_t", "wasm_ref_t", "PublicValue"):
            for access in ("*", ""):
                with self.subTest(name=name, access=access), tempfile.TemporaryDirectory() as directory:
                    root = pathlib.Path(directory)
                    for label, member in (("old", "int"), ("new", "long")):
                        source = root / f"{label}.c"
                        source.write_text(
                            f"struct {name} {{ {member} value; }};\n"
                            f"int api(struct {name} {access}value) {{ return sizeof(value); }}\n")
                        subprocess.run(["cc", "-g", "-O0", "-shared", "-fPIC", str(source),
                                        "-Wl,-soname,libtest.so.0", "-o", str(root / f"{label}.so")],
                                       check=True)
                    result = subprocess.run([
                        sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                        "--types", str(root / "old.so"), str(root / "new.so"),
                    ], capture_output=True, text=True)
                    expected = 0 if access == "*" and name != "PublicValue" else 1
                    self.assertEqual(result.returncode, expected, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
