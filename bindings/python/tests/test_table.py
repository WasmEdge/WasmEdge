import pytest
import WasmEdge


def test_table_type_cxt():
    lim = WasmEdge.Limit(True, 10, 20)
    t_cxt = WasmEdge.TableType(WasmEdge.RefType.FuncRef, lim)

    lim_api = t_cxt.GetLimit()
    assert lim_api.HasMax == lim.HasMax
    assert lim_api.Max == lim.Max
    assert lim_api.Min == lim.Min

    assert t_cxt.GetRefType() == WasmEdge.RefType.FuncRef


def test_table_inst():
    lim = WasmEdge.Limit(True, 10, 20)
    t_cxt = WasmEdge.TableType(WasmEdge.RefType.FuncRef, lim)

    table = WasmEdge.Table(t_cxt)
    t_cxt_api = table.GetType()

    lim_api = t_cxt_api.GetLimit()
    assert lim_api.HasMax == lim.HasMax
    assert lim_api.Max == lim.Max
    assert lim_api.Min == lim.Min

    assert t_cxt.GetRefType() == t_cxt_api.GetRefType()

    val = WasmEdge.Value(5, WasmEdge.Type.FuncRef)

    assert table.SetData(val, 3)

    res, data = table.GetData(3)

    assert res
    assert data.Type == val.Type
    assert data.Value == val.Value

    assert table.GetSize() == lim.Min
    assert table.GrowSize(5)
    assert table.GetSize() == lim.Min + 5
