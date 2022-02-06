import pytest
import WasmEdge


def test_host_function():
    log = WasmEdge.Logging()
    log.debug()

    store = WasmEdge.Store()
    vm = WasmEdge.VM(store)

    def add(a, b):
        res = WasmEdge.Value(a.Value + b.Value, WasmEdge.Type.I32)
        return WasmEdge.Result(0x00), [res]

    ftype = WasmEdge.FunctionType(
        [WasmEdge.Type.I32, WasmEdge.Type.I32], [WasmEdge.Type.I32]
    )
    func = WasmEdge.Function(ftype, add, 0)

    module_name = "extern"
    function_name = "func-add"

    mod = WasmEdge.ImportObject(module_name)
    mod.add(func, function_name)

    res = vm.RegisterModuleFromImport(mod)
    assert res

    mods = store.listModules()

    tup = (
        # /* WASM header */
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

    nums = [
        WasmEdge.Value(1234, WasmEdge.Type.I32),
        WasmEdge.Value(5678, WasmEdge.Type.I32),
    ]

    executor_function_name = "addTwo"

    res, l = vm.run(tup, nums, 1, executor_function_name)

    assert res

    assert l[0].Value == add(*tuple(nums))[1][0].Value

    assert len(vm.GetFunctionList(1)) == 1

    assert executor_function_name in vm.GetFunctionList(1).keys()

    assert mods[0] == module_name
