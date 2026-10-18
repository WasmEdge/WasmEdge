#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import pathlib
import re
import unittest

from abi_signatures import OPAQUE_TYPES


class InventoryTests(unittest.TestCase):
    def test_exact_public_opaque_declarations_are_covered(self):
        root = pathlib.Path(__file__).resolve().parents[2]
        context = (root / "include/api/wasmedge/wasmedge_context.h").read_text()
        wasm = (root / "thirdparty/wasm/wasm.h").read_text()
        context = re.sub(r"/\*.*?\*/|//[^\n]*", "", context, flags=re.S)
        wasm = re.sub(r"/\*.*?\*/|//[^\n]*", "", wasm, flags=re.S)
        expected = set(re.findall(r"typedef struct (WasmEdge_\w+)\s+\1\s*;", context))
        macros = re.findall(r"^WASM_DECLARE_(OWN|TYPE|REF|REF_BASE|SHARABLE_REF)\((\w+)\)", wasm, re.M)
        self.assertTrue(expected)
        self.assertTrue(macros)
        for kind, name in macros:
            expected.add(f"wasm_{name}_t")
            if kind == "SHARABLE_REF":
                expected.add(f"wasm_shared_{name}_t")
        self.assertEqual(OPAQUE_TYPES, expected)
        self.assertNotIn("wasm_val_t", OPAQUE_TYPES)
        self.assertNotIn("wasm_val_vec_t", OPAQUE_TYPES)


if __name__ == "__main__":
    unittest.main()
