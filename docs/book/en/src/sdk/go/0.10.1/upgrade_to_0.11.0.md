# Upgrade to WasmEdge-Go v0.11.0

Due to the WasmEdge-Go API breaking changes, this document shows the guideline of programming with WasmEdge-Go API to upgrade from the `v0.10.1` to the `v0.11.0` version.

## Concepts

1. Supported the user-defined error code in host functions.

    Developers can use the new API `wasmedge.NewResult()` to generate a `wasmedge.Result` struct with `wasmedge.ErrCategory_UserLevel` and the error code.
    With this support, developers can specify the host function error code when failed by themselves.
    For the examples to specify the user-defined error code, please refer to [the example below](#user-defined-error-code-in-host-functions).

2. Calling frame for the host function extension

    In the previous versions, host functions only pass the memory instance into the function body.
    For supporting the WASM multiple memories proposal and providing the recursive invocation in host functions, the new object `wasmedge.CallingFrame` replaced the memory instance in the second argument of the host function definition.
    For the examples of the new host function definition, please refer to [the example below](#calling-frame-in-host-functions).

## User Defined Error Code In Host Functions

Assume that we want to specify that the host function failed in the versions before `v0.10.1`:

```go
// Host function body definition.
func FaildFunc(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  return nil, wasmedge.Result_Fail
}
```

When the execution is finished, developers will get the `wasmedge.Result` struct if error occurred.
If developers call the `(*Result).GetCode()` with the returned error, they will get the value `2`.
If developers call the `(*Result).Error()` with the returned error, they will get the error string `"generic runtime error"`.

For the versions after `v0.11.0`, developers can specify the error code within 24-bit (smaller than `16777216`) size.

```go
// Host function body definition.
func FaildFunc(data interface{}, callframe *wasmedge.CallingFrame, params []interface{}) ([]interface{}, wasmedge.Result) {
  // This will create a trap in WASM with the error code.
  return nil, wasmedge.NewResult(wasmedge.ErrCategory_UserLevel, 12345678)
}
```

Therefore when developers call the `(*Result).GetCode()` with the returned error, they will get the error code `12345678`.
Noticed that if developers call the `(*Result).Error()`, they will always get the string `"user defined error code"`.

## Calling Frame In Host Functions

When implementing the host functions, developers usually use the input memory instance to load or store data.
In the WasmEdge versions before `v0.10.1`, the argument before the input and return value list of the host function definition is the memory instance object, so that developers can access the data in the memory instance.

```go
import (
  "encoding/binary"
  "fmt"
)

// Host function body definition.
func LoadOffset(data interface{}, mem *wasmedge.Memory, params []interface{}) ([]interface{}, wasmedge.Result) {
  // Function type: {i32} -> {}
  offset := params[0].(int32)
  data, err := mem.GetData(uint(offset), 4)
  if err != nil {
    return nil, err
  }
  fmt.Println("u32 at memory[{}]: {}", offset, binary.LittleEndian.Uint32(data))
  return nil, wasmedge.Result_Success
}
```

The input memory instance is the one that belongs to the module instance on the top calling frame of the stack.
However, after applying the WASM multiple memories proposal, there may be more than 1 memory instance in a WASM module.
Furthermore, there may be requests for accessing the module instance on the top frame of the stack to get the exported WASM functions, such as recursive invocation in host functions.
To support these, the `wasmedge.CallingFrame` is designed to replace the memory instance input of the host function.

In the WasmEdge versions after `v0.11.0`, the host function definitions are changed:

```go
type hostFunctionSignature func(data interface{}, callframe *wasmedge.CallingFrame, params []interface{}) ([]interface{}, wasmedge.Result)
```

Developers need to change to use the `wasmedge.CallingFrame` related APIs to access the memory instance:

```go
import (
  "encoding/binary"
  "fmt"
)

// Host function body definition.
func LoadOffset(data interface{}, callframe *wasmedge.CallingFrame, params []interface{}) ([]interface{}, wasmedge.Result) {
  // Function type: {i32} -> {}
  offset := params[0].(int32)

  // Get the 0th memory instance of the module of the top frame on the stack.
  mem := callframe.GetMemoryByIndex(0)

  data, err := mem.GetData(uint(offset), 4)
  if err != nil {
    return nil, err
  }
  fmt.Println("u32 at memory[{}]: {}", offset, binary.LittleEndian.Uint32(data))
  return nil, wasmedge.Result_Success
}
```

The `(*CallingFrame).GetModule()` API can help developers to get the module instance of the top frame on the stack.
With the module instance context, developers can use the module instance-related APIs to get its contents.

The `(*CallingFrame).GetExecutor()` API can help developers to get the currently used executor context.
Therefore developers can use the executor to recursively invoke other WASM functions without creating a new executor context.
