# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
"""gdb helpers for WasmEdge native debug info.

Load with: source utils/debug/wasmedge_gdb.py
"""
import gdb


def _frame_objfile(frame):
    try:
        symtab = frame.find_sal().symtab
    except gdb.error:
        return None
    return symtab.objfile if symtab is not None else None


def _memory_frame():
    frame = gdb.selected_frame()
    objfile = _frame_objfile(frame)
    while frame is not None:
        if objfile is not None and _frame_objfile(frame) != objfile:
            break
        try:
            return int(frame.read_var("__wasm_memory")), objfile
        except (ValueError, gdb.error):
            frame = frame.older()
    raise gdb.GdbError(
        "no __wasm_memory in the selected frame or its callers in the same "
        "module")


def _memory_base():
    return _memory_frame()[0]


def _lookup_symbol(objfile, name):
    if objfile is not None:
        for method in ("lookup_global_symbol", "lookup_static_symbol"):
            lookup = getattr(objfile, method, None)
            sym = lookup(name) if lookup is not None else None
            if sym is not None:
                return sym
    sym = gdb.lookup_global_symbol(name)
    if sym is None:
        sym = gdb.lookup_static_symbol(name)
    return sym


def _pointee_type(value):
    for field in value.type.strip_typedefs().fields():
        if field.name == "__pointee":
            return field.type.strip_typedefs().target().target()
    raise gdb.GdbError("value is not a __wasm_ptr")


def _host_pointer(value):
    addr = int(value["__addr"]) & 0xFFFFFFFF
    pointee = _pointee_type(value)
    raw = gdb.Value(_memory_base() + addr)
    return raw.cast(pointee.pointer())


def _debug_membase(objfile):
    sym = _lookup_symbol(objfile, "__wasmedge_debug_membase")
    if sym is not None:
        return int(sym.value())
    try:
        slot = gdb.parse_and_eval("&__wasmedge_debug_membase")
    except gdb.error:
        raise gdb.GdbError("no __wasmedge_debug_membase symbol")
    ulong = gdb.lookup_type("unsigned long")
    return int(slot.cast(ulong.pointer()).dereference())


class WasmPointer(gdb.Function):
    """$wasm(p): host pointer for the wasm pointer p."""

    def __init__(self):
        super().__init__("wasm")

    def invoke(self, value):
        return _host_pointer(value)


class WasmGlobal(gdb.Function):
    """$wasm_global("name"): value of a C global through the selected frame."""

    def __init__(self):
        super().__init__("wasm_global")

    def invoke(self, name):
        base, objfile = _memory_frame()
        sym = _lookup_symbol(objfile, name.string())
        if sym is None:
            raise gdb.GdbError("no global named " + name.string())
        value = sym.value()
        if value.is_optimized_out or value.address is None:
            raise gdb.GdbError(name.string() + " is not available")
        offset = int(value.address) - _debug_membase(objfile)
        return gdb.Value(base + offset).cast(
            sym.type.pointer()).dereference()


_printing_depth = 0


class WasmPointerPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        global _printing_depth
        try:
            addr = int(self.value["__addr"]) & 0xFFFFFFFF
        except gdb.error:
            return "(wasm) <unavailable>"
        if addr == 0 or _printing_depth > 0:
            return "(wasm) 0x%x" % addr
        _printing_depth += 1
        try:
            target = _host_pointer(self.value).dereference()
            return "(wasm) 0x%x -> %s" % (addr, target.format_string())
        except (gdb.error, gdb.GdbError):
            return "(wasm) 0x%x" % addr
        finally:
            _printing_depth -= 1


def _lookup(value):
    t = value.type.strip_typedefs()
    if t.code == gdb.TYPE_CODE_STRUCT and t.tag and \
            t.tag.startswith("__wasm_ptr"):
        return WasmPointerPrinter(value)
    return None


WasmPointer()
WasmGlobal()
gdb.pretty_printers.append(_lookup)
