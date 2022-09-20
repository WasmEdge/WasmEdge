### wasmedge-java

### Installation

### Build & install shared library
- Follow [build.md](https://github.com/WasmEdge/WasmEdge/blob/master/docs/build.md) to build and install WasmEdge
- Then go to `bindings/java/wasmedge-jni` directory
- Next run `mkdir build && cd build`
- Then run `cmake .. && make && make install`

### Build `wasmedge-java`
- Go to `bindings/java/wasmedge-java`
- Install gradle
- Run `gradle jar`

### How to use
- Add `build\libs\wasmedge-java.jar` as a dependency of your java project.

### How to run

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
  
  // Run a wasm file
  WasmEdgeAsync async = vm.asyncrunWasmFromFile("/root/fibonacci.wasm", "fib", params, returns);
  
  // Get return values
  async.wasmEdge_AsyncGet(returns);
  Assert.assertEquals(5, ((WasmEdgeI32Value) returns.get(0)).getValue());

```
