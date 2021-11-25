# Go

The best way to run Go programs is to create the application
in [TinyGo](https://tinygo.org/) and then [compile to WebAssembly](https://www.andreagrandi.it/2020/10/23/getting-started-with-tinygo-webassembly/).

```
tinygo build -o wasm.wasm -target wasm ./main.go
```

More to come later.
