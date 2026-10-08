# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
"""lldb helpers for WasmEdge native debug info.

Load with: command script import utils/debug/wasmedge_lldb.py
"""
import lldb

_MODULE = __name__


def _memory_base(frame):
    thread = frame.GetThread()
    index = frame.GetFrameID()
    while index < thread.GetNumFrames():
        v = thread.GetFrameAtIndex(index).FindVariable("__wasm_memory")
        if v.IsValid() and v.GetError().Success():
            return v.GetValueAsUnsigned()
        index += 1
    return None


def _debug_membase(target):
    error = lldb.SBError()
    addr = lldb.LLDB_INVALID_ADDRESS
    for ctx in target.FindSymbols("__wasmedge_debug_membase"):
        sym = ctx.GetSymbol()
        if sym.IsValid():
            addr = sym.GetStartAddress().GetLoadAddress(target)
            break
    if addr == lldb.LLDB_INVALID_ADDRESS:
        var = target.FindFirstGlobalVariable("__wasmedge_debug_membase")
        if var.IsValid():
            addr = var.GetLoadAddress()
    if addr == lldb.LLDB_INVALID_ADDRESS:
        return None
    value = target.GetProcess().ReadPointerFromMemory(addr, error)
    return value if error.Success() else None


def _is_wasm_ptr(value):
    return value.GetType().GetCanonicalType().GetName().startswith(
        "__wasm_ptr")


def _pointee_type(valobj):
    t = valobj.GetType().GetCanonicalType()
    for i in range(t.GetNumberOfFields()):
        f = t.GetFieldAtIndex(i)
        if f.GetName() == "__pointee":
            return f.GetType().GetArrayElementType().GetPointeeType()
    return None


def _host_value(valobj, frame):
    raw = valobj.GetNonSyntheticValue()
    base = _memory_base(frame)
    target = _pointee_type(raw)
    if base is None or target is None or not target.IsValid():
        return None
    addr = raw.GetChildMemberWithName("__addr").GetValueAsUnsigned()
    return raw.CreateValueFromAddress("*", base + (addr & 0xFFFFFFFF), target)


def wasm_ptr_summary(valobj, _dict):
    raw = valobj.GetNonSyntheticValue()
    addr = raw.GetChildMemberWithName("__addr").GetValueAsUnsigned()
    return "(wasm) 0x%x" % (addr & 0xFFFFFFFF)


class WasmPtrSynthetic:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.target = None

    def update(self):
        self.target = None
        raw = self.valobj.GetNonSyntheticValue()
        addr = raw.GetChildMemberWithName("__addr").GetValueAsUnsigned()
        frame = self.valobj.GetFrame()
        if addr & 0xFFFFFFFF and frame.IsValid():
            self.target = _host_value(raw, frame)
        return False

    def num_children(self):
        return 1 if self.target is not None and self.target.IsValid() else 0

    def get_child_index(self, name):
        return 0 if name == "*" else -1

    def get_child_at_index(self, index):
        return self.target if index == 0 and self.num_children() else None

    def has_children(self):
        return True


def _eval_path(frame, expr):
    parts = expr.replace("->", ".").split(".")
    value = frame.FindVariable(parts[0])
    for part in parts[1:]:
        if not value.IsValid():
            return None
        if _is_wasm_ptr(value):
            value = _host_value(value, frame)
            if value is None or not value.IsValid():
                return None
        value = value.GetChildMemberWithName(part)
    if value.IsValid() and _is_wasm_ptr(value):
        value = _host_value(value, frame)
    return value


def _selected_frame(debugger):
    return debugger.GetSelectedTarget().GetProcess().GetSelectedThread(
    ).GetSelectedFrame()


def wasm_command(debugger, command, result, _dict):
    frame = _selected_frame(debugger)
    expr = command.strip()
    value = _eval_path(frame, expr) if frame.IsValid() else None
    if value is None or not value.IsValid():
        result.SetError("cannot evaluate " + expr)
        return
    result.AppendMessage("%s = %s" % (expr, value.GetValue() or
                                      value.GetSummary() or str(value)))
    result.SetStatus(lldb.eReturnStatusSuccessFinishResult)


def wasm_global_command(debugger, command, result, _dict):
    target = debugger.GetSelectedTarget()
    frame = _selected_frame(debugger)
    name = command.strip()
    var = target.FindFirstGlobalVariable(name)
    membase = _debug_membase(target)
    base = _memory_base(frame) if frame.IsValid() else None
    if not var.IsValid() or membase is None or base is None:
        result.SetError("cannot read " + name)
        return
    offset = var.GetLoadAddress() - membase
    value = var.CreateValueFromAddress(name, base + offset, var.GetType())
    result.AppendMessage("%s = %s" % (name, value.GetValue() or str(value)))
    result.SetStatus(lldb.eReturnStatusSuccessFinishResult)


def __lldb_init_module(debugger, _dict):
    debugger.HandleCommand(
        "type summary add -e -x '^__wasm_ptr' -F %s.wasm_ptr_summary" % _MODULE)
    debugger.HandleCommand(
        "type synthetic add -x '^__wasm_ptr' -l %s.WasmPtrSynthetic" % _MODULE)
    debugger.HandleCommand(
        "command script add -f %s.wasm_command wasm" % _MODULE)
    debugger.HandleCommand(
        "command script add -f %s.wasm_global_command wasm-global" % _MODULE)
