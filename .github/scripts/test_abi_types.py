#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest


@unittest.skipUnless(
    sys.platform.startswith("linux")
    and all(shutil.which(tool) for tool in ("cc", "readelf", "abidiff")),
    "Requires Linux, cc, readelf and abidiff",
)
class TypeTests(unittest.TestCase):
    def test_missing_debug_information(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            source = root / "api.c"
            source.write_text("int api(void) { return 42; }\n")
            library = root / "api.so"
            subprocess.run(["cc", "-shared", "-fPIC", str(source),
                            "-Wl,-soname,libexample.so.0", "-o", str(library)], check=True)
            checker = pathlib.Path(__file__).with_name("check_abi.py")
            result = subprocess.run([sys.executable, str(checker), "--types",
                                     str(library), str(library)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn("Missing embedded DWARF", result.stderr)

    def test_same_symbol_signature_and_layout(self):
        cases = {
            "old": "struct Limit { int n; }; int api(struct Limit v) { return v.n; }",
            "same": "struct Limit { int n; }; int api(struct Limit v) { return v.n + 1; }",
            "signature": "struct Limit { int n; }; int api(struct Limit *v) { return v->n; }",
            "layout": "struct Limit { long n; }; int api(struct Limit v) { return v.n; }",
            "added": "struct Limit { int n; }; int api(struct Limit v) { return v.n; } void extra(void) {}",
        }
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            for name, code in cases.items():
                source = root / f"{name}.c"
                source.write_text(code + "\n")
                subprocess.run([
                    "cc", "-g", "-O0", "-shared", "-fPIC", str(source),
                    "-Wl,-soname,libexample.so.0", "-o", str(root / f"{name}.so"),
                ], check=True)
            checker = pathlib.Path(__file__).with_name("check_abi.py")
            for option in ("--types", "--types-only"):
                for name in cases:
                    with self.subTest(option=option, case=name):
                        result = subprocess.run([
                            sys.executable, str(checker), option,
                            str(root / "old.so"), str(root / f"{name}.so"),
                        ], capture_output=True, text=True)
                        if option == "--types-only":
                            self.assertNotIn("Preserved 0 ABI symbols", result.stdout)
                        expected = 1 if name in {"signature", "layout"} else 0
                        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
