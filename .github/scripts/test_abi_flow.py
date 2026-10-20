#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch

from check_release_abi import main


def release_metadata(tag):
    return {"tag_name": tag, "draft": False, "prerelease": False,
            "assets": [{"name": "WasmEdge-abi-linux-x86_64.json"},
                       {"name": "WasmEdge-abi-linux-x86_64.tar.gz"}]}


class FlowTests(unittest.TestCase):
    def test_latest_baseline_and_failures(self):
        for failure in ("none", "missing-marker", "missing-json", "missing-archive",
                        "duplicate-archive", "download", "checksum", "analyzer"):
            with self.subTest(failure=failure):
                releases = [release_metadata(tag) for tag in ("1.0", "1.1")]
                releases[0]["assets"] = []
                if failure == "missing-json":
                    releases[1]["assets"].pop(0)
                if failure == "missing-archive":
                    releases[1]["assets"].pop(1)
                if failure == "duplicate-archive":
                    releases[1]["assets"].append(releases[1]["assets"][1])
                downloads = []

                def command(args, **kwargs):
                    if args[0] == "gh":
                        return json.dumps([releases])
                    if args[1] == "rev-parse":
                        return "commit"
                    if args[1] == "ls-tree":
                        return "" if failure == "missing-marker" else "marker"
                    return "1"

                def download(tag, name, repository, directory):
                    self.assertEqual(tag, "1.1")
                    downloads.append(name)
                    if failure == "download":
                        raise subprocess.CalledProcessError(1, "gh")
                    path = Path(directory) / name
                    path.write_text(json.dumps({
                        "schema": 1, "version": tag, "commit": "commit",
                        "identity": ["ELF64", "little", "DYN (Shared object file)", "x86"],
                        "soname": "lib.so.0",
                        "sha256": "0" * 64 if failure == "checksum" else hashlib.sha256(b"ELF").hexdigest(),
                    }))
                    return path

                def inspect(path, option):
                    return {"--file-header": "Class: ELF64\nData: little\nType: DYN (Shared object file)\nMachine: x86\n",
                            "--dynamic": "(SONAME) [lib.so.0]"}[option]

                with patch.object(sys, "argv", ["check", "candidate.so", "1.2"]), \
                     patch("check_release_abi.select", return_value=[{"tag": "1.0"}, {"tag": "1.1"}]), \
                     patch("check_release_abi.subprocess.check_output", side_effect=command), \
                     patch("check_release_abi.download", side_effect=download), \
                     patch("check_release_abi.readelf", side_effect=inspect), \
                     patch("check_release_abi.library_from_archive", side_effect=lambda a, p: p.write_bytes(b"ELF")), \
                     patch("check_release_abi.subprocess.run") as analyze:
                    analyze.return_value.returncode = 1 if failure == "analyzer" else 0
                    if failure in {"none", "analyzer"}:
                        self.assertEqual(main(), int(failure == "analyzer"))
                        analyze.assert_called_once()
                        self.assertIn("--types-only", analyze.call_args.args[0])
                        self.assertEqual(len(downloads), 2)
                    else:
                        with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                            main()
                        analyze.assert_not_called()
                        if failure not in {"download", "checksum"}:
                            self.assertEqual(downloads, [])


if __name__ == "__main__":
    unittest.main()
