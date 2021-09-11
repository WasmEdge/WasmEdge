import pytest

from pywasmedge import WasmEdge, base_function

def test_base_class():
    assert WasmEdge().run() == "Hello World..."