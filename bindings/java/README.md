# wasmedge-java

## Prerequisites
- JDK 11 or later
- Gradle 6.3 or later

## Installation

### 1. Build & install shared library
- Follow [this link](https://wasmedge.org/book/en/contribute/build_from_src.html) to build and install WasmEdge

### 2. Build `wasmedge-java`
- Go to `bindings/java/wasmedge-java`
- Install gradle
- Run `./gradlew build`

### 3. How to use
- Add `build\libs\wasmedge-java.jar` as a dependency of your java project.

## Async API example

### WasmEdge Init
```java
  // Init wasmedge
  WasmEdge.init();

  // Create WasmEdgeVM
  WasmEdgeVM vm = new WasmEdgeVM();
  
  // Create param list
  List<WasmEdgeValue> params = new ArrayList<>();
  params.add(new WasmEdgeI32Value(4));

  // Create return list
  List<WasmEdgeValue> returns = new ArrayList<>();
  returns.add(new WasmEdgeI32Value());
```
### VM Run a wasm file 
```java
  WasmEdgeAsync async = vm.asyncrunWasmFromFile("/root/fibonacci.wasm", "fib", params, returns);
```
### VM Run a wasm from buffer
```java
  byte[] data = loadFile(getResourcePath(FIB_WASM_PATH));
  WasmEdgeAsync async = vm.asyncRunWasmFromBuffer(buffer, funcName, params);
```

### VM Run a wasm from AST module
```java
  ASTModuleContext mod = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));
  WasmEdgeAsync async = vm.asyncRunWasmFromBuffer(mod, funcName, params);
```

### VM　Run a wasm step by step
```java
  vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
  vm.validate();
  vm.instantiate();
  WasmEdgeAsync async = vm.execute("fib", params);
```

### VM Execute Register Module
```java
  String modName = ...;
  vm.registerModuleFromBuffer(modName, loadFile(getResourcePath(FIB_WASM_PATH)));
  WasmEdgeAsync async = vm.executeRegistered(modName, FUNC_NAME, params);
```


### Wait for the asynchronous execution
### Developers can wait the execution until finished
```java
  WasmEdgeAsync async = ...;
  async.wasmEdge_AsyncWait();

  // wasmEdge_AsyncDelete to delete and free the resource
  async.wasmEdge_AsyncDelete();
```
### Or developers can wait for a time limit.
```java
  WasmEdgeAsync async = ...;
  // Get return values
  boolean isEnd = async.wasmEdge_AsyncWaitFor(1000);
  if (IsEnd) {
    /* The execution finished. Developers can get the result. */
    async.wasmEdge_AsyncGet(returns);
  } else {
    /*
    * The time limit exceeded. Developers can keep waiting or cancel the execution.
    */
    async.wasmEdge_AsyncCancel();
    async.wasmEdge_AsyncGet(returns);
  }
  async.wasmEdge_AsyncDelete();
```

### Get the execution result of the asynchronous execution　
### Developers can use the wasmEdge_AsyncGetReturnsLength() API to get the return value list length. This function will block and wait for the execution. If the execution has finished, this function will return the length immediately. If the execution failed, this function will return 0. This function can help the developers to create the buffer to get the return values. If developers have already known the buffer length, they can skip this function and use the wasmEdge_AsyncGet() API to get the result.
```java
  WasmEdgeAsync async = ...;
  int len = async.wasmEdge_AsyncGetReturnsLength();
  async.wasmEdge_AsyncDelete();
```

### The wasmEdge_AsyncGet() API will block and wait for the execution. If the execution has finished, this function will fill the return values into the buffer and return the execution result immediately.

```java
  WasmEdgeAsync async = ...;
  // Create return list
  List<WasmEdgeValue> returns = new ArrayList<>();
  returns.add(new WasmEdgeI32Value());

  async.wasmEdge_AsyncGet(returns);
  async.wasmEdge_AsyncDelete();
```