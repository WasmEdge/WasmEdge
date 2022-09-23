# Call WasmEdge functions from an Android APK app

In this section, we will show you how to build a "regular" Android app (i.e., an APK file that can be installed on an Android device). The APK app embeds a WasmEdge Runtime. It can call WebAssembly functions through the embedded WasmEdge. The benefit is that developers can safely embed high-performance functions written in several different languages (e.g., Rust, JS, Grain, TinyGo etc) into a Kotlin application.

## Quickstart

The demo project is [available here](https://github.com/WasmEdge/WasmEdge/tree/master/utils/android/app). You can build the project using the Gradle tool or using the Android Stuido IDE.

### Building Project with Gradle

1. Setup environment variable `ANDROID_HOME=path/to/your/android/sdk`
2. Run Command `./gradlew assembleRelease`
3. Sign your APK file with `apksigner`. The apk file is at `./app/build/outputs/apk/release`. The `apksigner` utility is at `$ANDROID_HOME/build-tools/$VERSION/apksigner`.

### Building Project with Android Studio

Open this folder with [Android Studio](https://developer.android.com/studio) 2020.3.1 or later.

For Release APK, click `Menu -> Build -> Generate Signed Bundle/APK`, select APK, setup keystore configuration and wait for build finished.

## Review of the source code

The Android UI app is written in Kotlin, and it uses JNI (Java Native Interface) to load a C shared library, which in turn embeds WasmEdge.

### Android UI

The Android UI application is [located here](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/app/src/main/java/org/wasmedge/example_app/MainActivity.kt). It is written in Kotlin using the Android SDK.

```java
class MainActivity : AppCompatActivity() {
  lateinit var lib: NativeLib

  override fun onCreate(savedInstanceState: Bundle?) {
    super.onCreate(savedInstanceState)
    setContentView(R.layout.activity_main)

    val tv = findViewById<TextView>(R.id.tv_text)

    lib = NativeLib(this)

    Thread {
      val lines = Vector<String>()
      val idxArr = arrayOf(20, 25, 28, 30, 32)
      for (idx: Int in idxArr) {
        lines.add("running fib(${idx}) ...")
        runOnUiThread {
          tv.text = lines.joinToString("\n")
        }
        val begin = System.currentTimeMillis()
        val retVal = lib.wasmFibonacci(idx)
        val end = System.currentTimeMillis()
        lines.removeLast()
        lines.add("fib(${idx}) -> ${retVal}, ${end - begin}ms")
        runOnUiThread {
          tv.text = lines.joinToString("\n")
        }
      }
    }.start()
  }
}
```

### The native library

The Android UI app calls a `NativeLib` Kotlin object to access WasmEdge functions. The `NativeLib` source code is [available here](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/java/org/wasmedge/native_lib/NativeLib.kt). It uses JNI (Java Native Interface) to load a C shared library called `wasmedge_lib`. It then calls the `nativeWasmFibonacci` function in `wasmedge_lib` to execute the `fibonacci.wasm` WebAssembly bytecode.

```java
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
```

### The C shared library

The C shared library source code `wasmedge_lib.cpp` is [available here](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/cpp/wasmedge_lib.cpp). It uses the WasmEdge C SDK to embed a WasmEdge VM and execute the WebAssembly function.

```c
extern "C" JNIEXPORT jint JNICALL
Java_org_wasmedge_native_1lib_NativeLib_nativeWasmFibonacci(
    JNIEnv *env, jobject, jbyteArray image_bytes, jint idx) {
  jsize buffer_size = env->GetArrayLength(image_bytes);
  jbyte *buffer = env->GetByteArrayElements(image_bytes, nullptr);

  WasmEdge_ConfigureContext *conf = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(conf, WasmEdge_HostRegistration_Wasi);

  WasmEdge_VMContext *vm_ctx = WasmEdge_VMCreate(conf, nullptr);

  const WasmEdge_String &func_name = WasmEdge_StringCreateByCString("fib");
  std::array<WasmEdge_Value, 1> params{WasmEdge_ValueGenI32(idx)};
  std::array<WasmEdge_Value, 1> ret_val{};

  const WasmEdge_Result &res = WasmEdge_VMRunWasmFromBuffer(
      vm_ctx, (uint8_t *)buffer, buffer_size, func_name, params.data(),
      params.size(), ret_val.data(), ret_val.size());

  WasmEdge_VMDelete(vm_ctx);
  WasmEdge_ConfigureDelete(conf);
  WasmEdge_StringDelete(func_name);

  env->ReleaseByteArrayElements(image_bytes, buffer, 0);
  if (!WasmEdge_ResultOK(res)) {
    return -1;
  }
  return WasmEdge_ValueGetI32(ret_val[0]);
}
```

### The WebAssembly function

The `factorial.wat` is a [handwritten WebAssembly script](https://github.com/WasmEdge/WasmEdge/blob/master/examples/wasm/fibonacci.wat) to compute factorial numbers. It is compiled into WebAssembly using the [WABT tool](https://github.com/WebAssembly/wabt).

### Build dependencies

Android Studio and Gradle use CMake to build the C shared library. The [CMakeLists.txt file](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/cpp/CMakeLists.txt) builds the WasmEdge source into Android shared library files and embeds them into the final APK application. In this case, there is no seperate step to install WasmEdge share libraries onto the Android device.
