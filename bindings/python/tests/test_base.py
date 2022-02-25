import pytest
import WasmEdge
import os
import subprocess
import random
import unittest


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
    num = random.randint(2, 20)
    num_w = (WasmEdge.Value(num, WasmEdge.Type.I32),)
    res, l = vm.RunWasmFromFile(fib_wasm, "fib", num_w, 1)
    assert bool(res)
    assert l[0].Value == fibonacci(num)


def test_add():
    wasm_base_path = os.path.abspath(os.path.join(__file__, "../../../.."))
    add_wasm = os.path.join(wasm_base_path, "tools/wasmedge/examples/add.wasm")
    log = WasmEdge.Logging()
    log.debug()
    cfx = WasmEdge.Configure()
    cfx.add(WasmEdge.Host.Wasi)
    vm = WasmEdge.VM(cfx)
    nums = [random.randint(2, 20), random.randint(2, 20)]
    nums_w = tuple([WasmEdge.Value(x, WasmEdge.Type.I32) for x in nums])
    res, l = vm.RunWasmFromFile(add_wasm, "add", nums_w, 1)
    assert bool(res)
    assert l[0].Value == sum(nums)


def test_version():
    assert (
        "wasmedge version " + WasmEdge.VersionGet()
        == subprocess.run(
            ["wasmedge", "--version"], stdout=subprocess.PIPE
        ).stdout.decode("utf-8")[:-1]
    )


def test_value():
    num1 = 10
    num2 = 0.1
    num3 = 9223372036854775807

    def add(a, b):
        return a + b

    val = WasmEdge.Value(num1, WasmEdge.Type.I32)
    val2 = WasmEdge.Value(num2, WasmEdge.Type.F32)
    val3 = WasmEdge.Ref(WasmEdge.RefType.FuncRef)
    val4 = WasmEdge.Ref(WasmEdge.RefType.ExternRef, num1)
    val5 = WasmEdge.Ref(WasmEdge.RefType.ExternRef, add)
    val6 = WasmEdge.Ref(WasmEdge.RefType.FuncRef, num1)
    val7 = WasmEdge.Value(num3, WasmEdge.Type.I64)
    val8 = WasmEdge.Value(num1, WasmEdge.Type.FuncRef)
    val9 = WasmEdge.Value(num2, WasmEdge.Type.ExternRef)

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
    assert val7.Value == num3
    assert val7.Type == WasmEdge.Type.I64
    assert val8.Type == WasmEdge.Type.FuncRef
    assert val8.Value == 10
    assert val9.Value == num2
    assert val9.Type == WasmEdge.Type.ExternRef


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


def test_limit():

    lim = WasmEdge.Limit(True, 10, 20)
    assert lim.HasMax == True
    assert lim.Min == 10
    assert lim.Max == 20

    with unittest.TestCase.assertRaises(None, AttributeError):
        lim.Max = 30
