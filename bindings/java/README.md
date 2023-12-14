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
  List<Value> params = new ArrayList<>();
  params.add(new I32Value(4));

  // Create return list
  List<Value> returns = new ArrayList<>();
  returns.add(new I32Value());
```
### VM Run a wasm file 
```java
  String fibWasmPath = "/root/fibonacci.wasm";
  String funcName = "fib";
  
  Async async = vm.asyncRunWasmFromFile(fibWasmPath, funcName, params);
```
### VM Run a wasm from buffer
```java
  byte[] data = Files.readAllBytes(Paths.get(fibWasmPath));
  Async async = vm.asyncRunWasmFromBuffer(data, funcName, params);
```

### VM Run a wasm from AST module
```java
  LoaderContext loaderContext = new LoaderContext(null);
  AstModuleContext mod = loaderContext.parseFromFile(fibWasmPath);
  Async async = vm.asyncRunWasmFromAstModule(mod, funcName, params);
```

### VM　Run a wasm step by step
```java
  vm.loadWasmFromFile(fibWasmPath);
  vm.validate();
  vm.instantiate();
  vm.execute(funcName, params, returns);
```

### VM Execute Register Module
```java
  String modName = "fibonacciModule";
  byte[] data = Files.readAllBytes(Paths.get(fibWasmPath));
  vm.registerModuleFromBuffer(modName, data);
  vm.executeRegistered(modName, funcName, params, returns);
```


### Wait for the asynchronous execution
### Developers can wait the execution until finished
```java
  WasmEdgeAsync async = ...;
  async.asyncWait();

  // close to delete and free the resource
  async.close();
```
### Or developers can wait for a time limit.
```java
  WasmEdgeAsync async = ...;
  // Get return values
  boolean isEnd = async.waitFor(1000);
  if (IsEnd) {
    /* The execution finished. Developers can get the result. */
    async.get(returns);
  } else {
    /*
    * The time limit exceeded. Developers can keep waiting or cancel the execution.
    */
    async.cancel();
    async.get(returns);
  }
  async.close();
```

### Get the execution result of the asynchronous execution　
### Developers can use the getReturnsLength() API to get the return value list length. This function will block and wait for the execution. If the execution has finished, this function will return the length immediately. If the execution failed, this function will return 0. This function can help the developers to create the buffer to get the return values. If developers have already known the buffer length, they can skip this function and use the get() API to get the result.
```java
  WasmEdgeAsync async = ...;
  int len = async.getReturnsLength();
  async.close();
```

### The get() API will block and wait for the execution. If the execution has finished, this function will fill the return values into the buffer and return the execution result immediately.

```java
  WasmEdgeAsync async = ...;
  // Create return list
  List<WasmEdgeValue> returns = new ArrayList<>();
  returns.add(new WasmEdgeI32Value());

  async.get(returns);
  async.close();
```
