from re import A
import WasmEdge
import pytest
import os
import random


def fibonacci(n):
    a = 0
    b = 1

    if n < 0:
        raise Exception("Incorrect input")
    elif n == 0:
        return 0
    elif n == 1:
        return b
    else:
        for i in range(1, n + 1):
            c = a + b
            a = b
            b = c
        return b


def test_execute_registered():
    fib_wasm = os.path.join(
        os.path.abspath(os.path.join(__file__, "../../../..")),
        "tools/wasmedge/examples/fibonacci.wasm",
    )
    mod_name = "mod"
    vm = WasmEdge.VM()
    num = random.randint(2, 20)
    num_w = WasmEdge.Value(num, WasmEdge.Type.I32)
    res = vm.RegisterModuleFromFile(mod_name, fib_wasm)
    res, l = vm.ExecuteRegistered(mod_name, "fib", [num_w], 1)
    assert res
    assert l[0].Value == fibonacci(num)
