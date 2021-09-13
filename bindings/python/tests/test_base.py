import pytest
import WasmEdge
import os


def test_fib_32():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    fib_wasm = os.path.join(
        wasm_base_path, "tools/wasmedge/examples/fibonacci.wasm"
    )
    assert (
        WasmEdge.py_run(
            fib_wasm,
            "fib",
        )
        == 3524578
    )
