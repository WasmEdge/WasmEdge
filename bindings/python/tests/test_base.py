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


def test_value():
    num1 = 10
    num2 = 0.1

    def add(a, b):
        return a + b

    val = WasmEdge.Value(num1)
    val2 = WasmEdge.Value(num2)
    val3 = WasmEdge.Ref(WasmEdge.RefType.FuncRef)
    val4 = WasmEdge.Ref(WasmEdge.RefType.ExternRef, num1)
    val5 = WasmEdge.Ref(WasmEdge.RefType.ExternRef, add)
    val6 = WasmEdge.Ref(WasmEdge.RefType.FuncRef, num1)

    assert val.Value * val2.Value == num1 * num2
    assert val.Type == WasmEdge.Type.I32
    assert val2.Type == WasmEdge.Type.F32
    assert val3.Type == WasmEdge.RefType.FuncRef
    assert val3.isNull() == True
    assert val4.Type == WasmEdge.RefType.ExternRef
    assert val4.Value == num1
    assert val5.Type == WasmEdge.RefType.ExternRef
    assert val5.Value(num1, num2) == num1 + num2
    assert val6.Type == WasmEdge.RefType.FuncRef
    assert val6.FuncIdx() == num1
    assert val6.Value == num1

    del num1
    assert val4.Value == 10


def test_step_by_step():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    path = os.path.join(
        wasm_base_path, "tools/wasmedge/examples/fibonacci.wasm"
    )
    log = WasmEdge.Logging()
    log.debug()

    cfx = WasmEdge.Configure()
    store = WasmEdge.Store()
    loader = WasmEdge.Loader(cfx)
    validator = WasmEdge.Validator(cfx)
    executor = WasmEdge.Executor(cfx)
    ast = WasmEdge.ASTModule()

    num = [18]

    assert loader.parse(ast, path)

    assert validator.validate(ast)

    assert executor.instantiate(store, ast)

    func_name = "fib"
    res, l = executor.invoke(store, func_name, num)
    assert res

    assert l[0] == fibonacci(num[0])
