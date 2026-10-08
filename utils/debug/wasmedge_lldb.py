# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors
"""lldb helpers for WasmEdge native debug info.

Load with: command script import utils/debug/wasmedge_lldb.py
"""
import lldb

_MODULE = __name__


def _frame_memory(frame):
    v = frame.FindVariable("__wasm_memory")
    if v.IsValid() and v.GetError().Success():
        return v.GetValueAsUnsigned()
    return None


def _memory_frame(frame):
    thread = frame.GetThread()
    index = frame.GetFrameID()
    while frame.IsValid():
        memory = _frame_memory(frame)
        if memory is not None:
            return memory, frame
        if not frame.IsInlined():
            break
        index += 1
        frame = thread.GetFrameAtIndex(index)
    return None, None


def _memory_base(frame):
    return _memory_frame(frame)[0]


def _debug_membase(target, module):
    name = "__wasmedge_debug_membase"
    error = lldb.SBError()
    addr = lldb.LLDB_INVALID_ADDRESS
    for ctx in module.FindSymbols(name):
        sym = ctx.GetSymbol()
        if sym.IsValid():
            addr = sym.GetStartAddress().GetLoadAddress(target)
            break
    if addr == lldb.LLDB_INVALID_ADDRESS:
        var = module.FindFirstGlobalVariable(target, name)
        if var.IsValid():
            addr = var.GetLoadAddress()
    if addr == lldb.LLDB_INVALID_ADDRESS:
        return None
    value = target.GetProcess().ReadPointerFromMemory(addr, error)
    return value if error.Success() else None


def _module_global(target, module, name):
    """Return a C global of the module and the memory base that its address
    is relative to. A constant has no address and needs no memory base."""
    var = module.FindFirstGlobalVariable(target, name)
    if not var.IsValid():
        return None, None
    if var.GetLoadAddress() == lldb.LLDB_INVALID_ADDRESS:
        return (var, None) if var.GetValue() is not None else (None, None)
    membase = _debug_membase(target, module)
    return (var, membase) if membase is not None else (None, None)


def _module_id(target, module):
    var = module.FindFirstGlobalVariable(target, "__wasmedge_debug_module")
    if not var.IsValid():
        return None
    error = lldb.SBError()
    value = var.GetValueAsUnsigned(error, 0)
    return value if error.Success() else None


def _find_global(target, module, name):
    var, membase = _module_global(target, module, name)
    if var is not None:
        return var, membase
    module_id = _module_id(target, module)
    if module_id is None:
        return None, None
    for other in target.module_iter():
        if other == module or _module_id(target, other) != module_id:
            continue
        var, membase = _module_global(target, other, name)
        if var is not None:
            return var, membase
    return None, None


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


def _inside_pointee(valobj):
    path = lldb.SBStream()
    valobj.GetExpressionPath(path)
    return (path.GetData() or "").startswith("*")


def _wasm_addr(valobj):
    """Return the wasm address, or None if lldb cannot read the pointer."""
    raw = valobj.GetNonSyntheticValue()
    error = lldb.SBError()
    addr = raw.GetChildMemberWithName("__addr").GetValueAsUnsigned(error, 0)
    return addr & 0xFFFFFFFF if error.Success() else None


def _host_value(valobj, frame):
    raw = valobj.GetNonSyntheticValue()
    addr = _wasm_addr(raw)
    base = _memory_base(frame)
    target = _pointee_type(raw)
    if not addr or base is None or target is None or not target.IsValid():
        return None
    return raw.CreateValueFromAddress("*", base + addr, target)


def wasm_ptr_summary(valobj, _dict):
    addr = _wasm_addr(valobj)
    if addr is None:
        return "(wasm) <unavailable>"
    return "(wasm) 0x%x" % addr


class WasmPtrSynthetic:
    def __init__(self, valobj, _dict):
        self.valobj = valobj
        self.target = None

    def update(self):
        self.target = None
        raw = self.valobj.GetNonSyntheticValue()
        frame = self.valobj.GetFrame()
        if not frame.IsValid() or _inside_pointee(raw):
            return False
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
    var, membase = (_find_global(target, frame.GetModule(), name)
                    if frame.IsValid() else (None, None))
    base = _memory_base(frame) if membase is not None else None
    if var is None or (membase is not None and base is None):
        result.SetError("cannot read " + name)
        return
    if membase is None:
        value = var
    else:
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
