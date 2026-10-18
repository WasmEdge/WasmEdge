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
class ShimTests(unittest.TestCase):
    def test_versioned_compatibility_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            versions = root / "versions.map"
            versions.write_text("V1 { global: api; local: *; }; V2 { global: api; } V1;\n")
            sources = {
                "old": 'struct Limit { int value; }; int old_api(struct Limit v) {return v.value;} __asm__(".symver old_api,api@@V1");',
                "compatible": 'struct Limit_Compat { int value; }; int old_api(struct Limit_Compat v) {return v.value;} int new_api(const struct Limit_Compat *v) {return v->value;} __asm__(".symver old_api,api@V1"); __asm__(".symver new_api,api@@V2");',
                "broken": 'struct Limit_Compat { long value; }; int old_api(struct Limit_Compat v) {return v.value;} int new_api(const struct Limit_Compat *v) {return v->value;} __asm__(".symver old_api,api@V1"); __asm__(".symver new_api,api@@V2");',
            }
            for name, source in sources.items():
                path = root / f"{name}.c"
                path.write_text(source + "\n")
                subprocess.run(["cc", "-g", "-O0", "-shared", "-fPIC", str(path),
                                "-Wl,-soname,libtest.so.0", f"-Wl,--version-script={versions}",
                                "-o", str(root / f"{name}.so")], check=True)
            for name, expected in (("compatible", 0), ("broken", 1)):
                with self.subTest(name=name):
                    result = subprocess.run([
                        sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                        "--types-only", str(root / "old.so"), str(root / f"{name}.so"),
                    ], capture_output=True, text=True)
                    self.assertEqual(result.returncode, expected, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
