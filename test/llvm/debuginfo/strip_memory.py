#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
"""Remove linear memory from a wasm module, keeping other sections intact."""
import sys

MEMORY, DATA, DATACOUNT, IMPORT, EXPORT = 5, 11, 12, 2, 7


def readLeb(Buf, Pos):
    Result = Shift = 0
    while True:
        Byte = Buf[Pos]
        Pos += 1
        Result |= (Byte & 0x7F) << Shift
        if not Byte & 0x80:
            return Result, Pos
        Shift += 7


def writeLeb(Value):
    Out = bytearray()
    while True:
        Byte = Value & 0x7F
        Value >>= 7
        if Value:
            Out.append(Byte | 0x80)
        else:
            Out.append(Byte)
            return bytes(Out)


def checkImports(Body):
    Count, Pos = readLeb(Body, 0)
    for _ in range(Count):
        for _ in range(2):
            Len, Pos = readLeb(Body, Pos)
            Pos += Len
        Kind = Body[Pos]
        Pos += 1
        if Kind == 2:
            raise SystemExit("strip_memory: module imports a memory")
        if Kind == 0:
            _, Pos = readLeb(Body, Pos)
        elif Kind == 1:
            Pos += 1
            Flag = Body[Pos]
            Pos += 1
            _, Pos = readLeb(Body, Pos)
            if Flag & 1:
                _, Pos = readLeb(Body, Pos)
        elif Kind == 3:
            Pos += 2
        else:
            raise SystemExit("strip_memory: unsupported import kind %d" % Kind)


def stripExports(Body):
    Count, Pos = readLeb(Body, 0)
    Kept = []
    for _ in range(Count):
        Start = Pos
        Len, Pos = readLeb(Body, Pos)
        Pos += Len
        Kind = Body[Pos]
        Pos += 1
        _, Pos = readLeb(Body, Pos)
        if Kind != 2:
            Kept.append(bytes(Body[Start:Pos]))
    if Pos != len(Body):
        raise SystemExit("strip_memory: malformed export section")
    return writeLeb(len(Kept)) + b"".join(Kept)


def main(Src, Dst):
    Data = open(Src, "rb").read()
    if Data[:8] != b"\0asm\1\0\0\0":
        raise SystemExit("strip_memory: not a wasm module")
    Out = bytearray(Data[:8])
    Pos = 8
    Dropped = 0
    while Pos < len(Data):
        Start = Pos
        Id = Data[Pos]
        Size, Pos = readLeb(Data, Pos + 1)
        Body = Data[Pos : Pos + Size]
        Pos += Size
        if Id in (MEMORY, DATA, DATACOUNT):
            Dropped += 1
        elif Id == IMPORT:
            checkImports(Body)
            Out += Data[Start:Pos]
        elif Id == EXPORT:
            NewBody = stripExports(Body)
            Out += bytes([Id]) + writeLeb(len(NewBody)) + NewBody
        else:
            Out += Data[Start:Pos]
    if not Dropped:
        raise SystemExit("strip_memory: no memory section found")
    open(Dst, "wb").write(Out)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: strip_memory.py in.wasm out.wasm")
    main(sys.argv[1], sys.argv[2])
