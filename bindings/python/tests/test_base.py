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
    res, l = vm.run(
        fib_wasm, "fib", [num], [WasmEdge.Type.I32], [WasmEdge.Type.I32]
    )
    assert bool(res)
    assert l[0] == fibonacci(num)


def test_add():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    add_wasm = os.path.join(wasm_base_path, "tools/wasmedge/examples/add.wasm")
    log = WasmEdge.Logging()
    log.debug()
    cfx = WasmEdge.Configure()
    cfx.add(WasmEdge.Host.Wasi)
    vm = WasmEdge.VM(cfx)
    nums = [random.randint(2, 20), random.randint(2, 20)]
    res, l = vm.run(add_wasm, "add", nums)
    assert bool(res)
    assert l[0] == sum(nums)


def test_version():
    assert (
        "wasmedge version " + WasmEdge.version()
        == subprocess.run(
            ["wasmedge", "--version"], stdout=subprocess.PIPE
        ).stdout.decode("utf-8")[:-1]
    )


def test_host_function():
    log = WasmEdge.Logging()
    log.debug()
    vm = WasmEdge.VM()

    def add(a: int, b: int) -> int:
        return a + b

    func = WasmEdge.Function(add)
    mod = WasmEdge.Module("extern")
    mod.add(func, "func-add")
    vm.add(mod)

    tup = (  # /* WASM header */
        0x00,
        0x61,
        0x73,
        0x6D,
        0x01,
        0x00,
        0x00,
        0x00,
        # /* Type section */
        0x01,
        0x07,
        0x01,
        # /* function type {i32, i32} -> {i32} */
        0x60,
        0x02,
        0x7F,
        0x7F,
        0x01,
        0x7F,
        # /* Import section */
        0x02,
        0x13,
        0x01,
        # /* module name: "extern" */
        0x06,
        0x65,
        0x78,
        0x74,
        0x65,
        0x72,
        0x6E,
        # /* extern name: "func-add" */
        0x08,
        0x66,
        0x75,
        0x6E,
        0x63,
        0x2D,
        0x61,
        0x64,
        0x64,
        # /* import desc: func 0 */
        0x00,
        0x00,
        # /* Function section */
        0x03,
        0x02,
        0x01,
        0x00,
        # /* Export section */
        0x07,
        0x0A,
        0x01,
        # /* export name: "addTwo" */
        0x06,
        0x61,
        0x64,
        0x64,
        0x54,
        0x77,
        0x6F,
        # /* export desc: func 0 */
        0x00,
        0x01,
        # /* Code section */
        0x0A,
        0x0A,
        0x01,
        # /* code body */
        0x08,
        0x00,
        0x20,
        0x00,
        0x20,
        0x01,
        0x10,
        0x00,
        0x0B,
    )

    nums = [1234, 5678]

    res, l = vm.run(tup, nums, "addTwo", 1)

    assert res

    assert l[0] == add(*tuple(nums))
