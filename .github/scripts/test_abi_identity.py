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
class IdentityTests(unittest.TestCase):
    def test_opaque_identity_changes(self):
        definitions = "struct WasmEdge_VMContext { int x; }; struct WasmEdge_ConfigureContext { long x; };\n"
        cases = {
            "parameter": ("int api(struct WasmEdge_VMContext *p) {return p->x;}",
                          "int api(struct WasmEdge_ConfigureContext *p) {return p->x;}"),
            "return": ("struct WasmEdge_VMContext *api(void) {return 0;}",
                       "struct WasmEdge_ConfigureContext *api(void) {return 0;}"),
            "typedef": ("typedef struct WasmEdge_VMContext Handle; int api(Handle *p) {return p->x;}",
                        "typedef struct WasmEdge_ConfigureContext Handle; int api(Handle *p) {return p->x;}"),
        }
        cases["wasm-ref-target"] = (
            "struct wasm_ref_t { int x; }; int api(struct wasm_ref_t *p) { return p->x; }",
            "struct wasm_engine_t { int x; }; int api(struct wasm_engine_t *p) { return p->x; }",
        )
        for kind in ("member", "callback"):
            sources = []
            for context in ("WasmEdge_VMContext", "WasmEdge_ConfigureContext"):
                member = (f"struct {context} *value;" if kind == "member" else
                          f"int (*callback)(struct {context} *);")
                sources.append(f"struct Public {{ {member} }}; int api(struct Public *p) {{ return sizeof(*p); }}")
            cases[kind] = sources
        for name, sources in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as directory:
                root = pathlib.Path(directory)
                for label, source in zip(("old", "new"), sources):
                    path = root / f"{label}.c"
                    path.write_text(definitions + source + "\n")
                    subprocess.run(["cc", "-g", "-O0", "-shared", "-fPIC", str(path),
                                    "-Wl,-soname,libtest.so.0", "-o", str(root / f"{label}.so")], check=True)
                result = subprocess.run([
                    sys.executable, str(pathlib.Path(__file__).with_name("check_abi.py")),
                    "--types", str(root / "old.so"), str(root / "new.so"),
                ], capture_output=True, text=True)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertIn("type identity changed", result.stdout)


if __name__ == "__main__":
    unittest.main()
