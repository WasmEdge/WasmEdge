#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
"""Convert between binary files and the fixture hex text format."""
import binascii
import sys


def to_hex(src, dst):
    with open(src, "rb") as f:
        text = binascii.hexlify(f.read()).decode("ascii")
    with open(dst, "w", newline="\n") as f:
        for i in range(0, len(text), 64):
            f.write(text[i:i + 64] + "\n")


def from_hex(src, dst):
    with open(src, "r") as f:
        data = binascii.unhexlify("".join(f.read().split()))
    with open(dst, "wb") as f:
        f.write(data)


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ("--to-hex", "--from-hex"):
        sys.exit("usage: hex_tool.py (--to-hex|--from-hex) SRC DST")
    (to_hex if sys.argv[1] == "--to-hex" else from_hex)(sys.argv[2],
                                                         sys.argv[3])


if __name__ == "__main__":
    main()
