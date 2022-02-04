import pytest
import WasmEdge
import unittest


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


def test_memory_instance():
    lim = WasmEdge.Limit(True, 1, 5)
    memory_type = WasmEdge.MemoryType(lim)
    mem_inst = WasmEdge.Memory(memory_type)

    data = (0xAA, 0xBB, 0xCC)

    assert mem_inst.SetData(data, 0x1000)

    res, new_data = mem_inst.GetData(len(data), 0x1000)

    assert res
    assert tuple(new_data) == data

    assert 1 == mem_inst.GetPageSize()
    assert mem_inst.GrowPage(2)
    assert 2 + 1 == mem_inst.GetPageSize()
    assert mem_inst.GrowPage(2)
    assert 2 + 2 + 1 == mem_inst.GetPageSize()
    with unittest.TestCase.assertRaises(None, AssertionError):
        assert mem_inst.GrowPage(1)
