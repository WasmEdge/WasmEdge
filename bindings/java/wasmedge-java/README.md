### wasmedge-java

### Installation

### Build & install shared library
- Follow [build.md](https://github.com/WasmEdge/WasmEdge/blob/master/docs/build.md) to build and install WasmEdge
- Then go to `bindings/java/wasmedge-java` directory
- Next run `mkdir build && cmake ..`
- Then run `make && make install`

### Build `wasmedge-java`
- Go to `bindings/java/wasmedge-java`
- Install gradle
- Run `gradle jar`

### How to use
- Add `build\libs\wasmedge-java.jar` as a dependency of your java project.
