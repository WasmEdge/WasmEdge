package main

import (
	"fmt"
	"os"

	"github.com/second-state/WasmEdge-go/wasmedge"
)

func main() {
	fmt.Println("Go: Args:", os.Args)
	/// Expected Args[0]: program name (./wasmAOT)
	/// Expected Args[1]: wasm file path to compile
	/// Expected Args[2]: output compiled-wasm path

	/// Set not to print debug info
	wasmedge.SetLogErrorLevel()

	/// Create Compiler
	var compiler = wasmedge.NewCompiler()

	/// Compile WASM AOT
	err := compiler.Compile(os.Args[1], os.Args[2])
	if err != nil {
		fmt.Println("Go: Compile WASM to AOT mode Failed!!")
	}

	compiler.Release()
}
