import pytest
import WasmEdge
import os
import subprocess
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


def test_fib_32():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    fib_wasm = os.path.join(
        wasm_base_path, "tools/wasmedge/examples/fibonacci.wasm"
    )
    cfx = WasmEdge.Configure()
    vm = WasmEdge.VM(cfx)
    # vm = WasmEdge.VM()
    num = random.randint(2, 20)
    res, l = vm.run(fib_wasm, "fib", [num], 1)
    assert bool(res)
    assert l[0] == str(fibonacci(num))


def test_add():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    add_wasm = os.path.join(wasm_base_path, "tools/wasmedge/examples/add.wasm")
    log = WasmEdge.Logging()
    log.debug()
    cfx = WasmEdge.Configure()
    vm = WasmEdge.VM(cfx)
    nums = [random.randint(2, 20), random.randint(2, 20)]
    res, l = vm.run(add_wasm, "add", nums)
    assert bool(res)
    assert l[0] == str(sum(nums))


def test_version():
    assert (
        "wasmedge version " + WasmEdge.version()
        == subprocess.run(
            ["wasmedge", "--version"], stdout=subprocess.PIPE
        ).stdout.decode("utf-8")[:-1]
    )
