#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

from save_abi_baseline import save


class SaveTests(unittest.TestCase):
    def test_first_baseline_and_failures(self):
        for mode in ("success", "no-debug", "analyzer-error"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                library = root / "library.so"
                library.write_bytes(b"fixture")
                cache = root / "CMakeCache.txt"
                cache.write_text("CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++\n"
                                 "CMAKE_BUILD_TYPE:STRING=Release\n"
                                 "CMAKE_CXX_FLAGS:STRING=-g\n")
                output = root / "output"

                def inspect(path, option):
                    return {
                        "--sections": "" if mode == "no-debug" else ".debug_info",
                        "--file-header": "Class: ELF64\nData: little endian\nType: DYN (Shared object file)\nMachine: X86-64\n",
                        "--dynamic": "(SONAME) [libexample.so.0]",
                        "--dyn-syms": "1: 0001 1 FUNC GLOBAL DEFAULT 1 api@@V1",
                    }[option]

                with patch("save_abi_baseline.readelf", side_effect=inspect), \
                     patch("save_abi_baseline.subprocess.check_output", return_value="verified\n") as command, \
                     patch("save_abi_baseline.subprocess.run") as run:
                    if mode == "analyzer-error":
                        run.side_effect = subprocess.CalledProcessError(1, "abidiff")
                    if mode != "success":
                        with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                            save(library, "1.0", output, cache)
                        self.assertFalse(output.exists())
                        continue
                    save(library, "1.0", output, cache)
                    data = json.loads((output / "WasmEdge-abi-linux-x86_64.json").read_text())
                    self.assertEqual(data["version"], "1.0")
                    self.assertEqual(data["commit"], "verified")
                    self.assertEqual(data["compiler"]["path"], "/usr/bin/clang++")
                    self.assertEqual(data["sha256"], hashlib.sha256(b"fixture").hexdigest())
                    with tarfile.open(output / "WasmEdge-abi-linux-x86_64.tar.gz") as archive:
                        self.assertEqual(archive.extractfile("libwasmedge.so.debug").read(), b"fixture")
                    self.assertEqual(command.call_args_list[0].args[0][0], "/usr/bin/clang++")


if __name__ == "__main__":
    unittest.main()
