import WasmEdge
import pytest


def test_function_type():
    param_types = [WasmEdge.Type.I32, WasmEdge.Type.F32, WasmEdge.Type.FuncRef]
    ret_types = [WasmEdge.Type.I64, WasmEdge.Type.F64, WasmEdge.Type.ExternRef]
    ftype = WasmEdge.FunctionType(param_types, ret_types)

    assert ftype.GetParamLen() == len(param_types)
    assert ftype.GetRetLen() == len(ret_types)

    ret_types_api = ftype.GetRetTypes(len(ret_types))
    param_types_api = ftype.GetParamTypes(len(param_types))

    assert all([x == y for x, y in zip(param_types_api, param_types)])
    assert all([x == y for x, y in zip(ret_types_api, ret_types)])


def test_function():
    param_types = [WasmEdge.Type.I32, WasmEdge.Type.F32, WasmEdge.Type.FuncRef]
    ret_types = [WasmEdge.Type.I32]
    ftype = WasmEdge.FunctionType(param_types, ret_types)

    def func(a, b):
        return WasmEdge.Result(0), WasmEdge.Value(a + b, WasmEdge.Type.I32)

    func_inst = WasmEdge.Function(ftype, func, 0)

    ftype_api = func_inst.GetType()

    assert ftype_api.GetParamLen() == len(param_types)
    assert ftype_api.GetRetLen() == len(ret_types)

    ret_types_api = ftype_api.GetRetTypes(len(ret_types))
    param_types_api = ftype_api.GetParamTypes(len(param_types))

    assert all([x == y for x, y in zip(param_types_api, param_types)])
    assert all([x == y for x, y in zip(ret_types_api, ret_types)])
