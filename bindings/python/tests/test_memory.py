import pytest
import WasmEdge


def test_memory_type():
    lim = WasmEdge.Limit(True, 10, 20)
    memory_type = WasmEdge.MemoryType(lim)
    lim_api = memory_type.GetLimit()
    assert lim_api.HasMax == lim.HasMax
    assert lim_api.Max == lim.Max
    assert lim_api.Min == lim.Min


def test_global_type():
    global_type = WasmEdge.GlobalType(
        WasmEdge.Type.I32, WasmEdge.Mutability.Var
    )
    assert WasmEdge.Mutability.Var == global_type.GetMutability()
    assert WasmEdge.Type.I32 == global_type.GetValType()
