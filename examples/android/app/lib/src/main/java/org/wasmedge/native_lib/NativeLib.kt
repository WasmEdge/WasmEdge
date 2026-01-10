package org.wasmedge.native_lib

import android.content.Context

class NativeLib(ctx : Context) {
    private external fun nativeWasmFibonacci(imageBytes : ByteArray, idx : Int ) : Int

    companion object {
        init {
            System.loadLibrary("wasmedge_lib")
        }
    }

    private var fibonacciWasmImageBytes : ByteArray = ctx.assets.open("fibonacci.wasm").readBytes()

    fun wasmFibonacci(idx : Int) : Int{
        return nativeWasmFibonacci(fibonacciWasmImageBytes, idx)
    }
}
