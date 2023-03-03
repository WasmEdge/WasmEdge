# 从 Android APK 应用调用 WasmEdge 函数

在本节中，我们将向您展示如何构建 Android Package（即可以安装在 Android 设备上的 APK 文件）。 APK 嵌入了 WasmEdge 运行时，即它可以通过嵌入的 WasmEdge 调用 WebAssembly 函数。这使开发人员可以安全地将用几种不同语言（例如，Rust、JS、Grain、TinyGo 等）编写的高性能函数嵌入到 Kotlin 应用程序中。

## 快速开始

示例项目 [可在此处获得](https://github.com/WasmEdge/WasmEdge/tree/master/utils/android/app)。您可以使用 Gradle 工具或 Android Studio IDE 构建项目。

### 使用 Gradle 构建项目

1. 设置环境变量 `ANDROID_HOME=path/to/your/android/sdk`
2. 运行 Gradlew 构建 `./gradlew assembleRelease`
3. 使用 Android 签名工具 `apksigner` 为 APK 文件签名。APK 文件位于 `./app/build/outputs/apk/release`，而 `apksigner` 工具位于 `$ANDROID_HOME/build-tools/$VERSION/apksigner`。

### 使用 Android Studio 构建项目

使用 2020.3.1 以后版本的 [Android Studio](https://developer.android.com/studio) 打开示例项目文件夹。

对于 Release APK，点击 `Menu -> Build -> Generate Signed Bundle/APK`，选择 APK，设置 keystore 配置并等待构建完成。

## 示例源代码

Android UI 应用程序是用 Kotlin 编写的，它使用 JNI (Java Native Interface) 来加载一个 C 共享库，该库又嵌入了 WasmEdge 。

### Android UI

Android UI 源码 [位于此处](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/app/src/main/java/org/wasmedge/example_app/MainActivity.kt)。它是使用 Android SDK 用 Kotlin 编写的。

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

### 原生库

Android UI 应用调用一个 Kotlin 语言编写的 `NativeLib` 对象来访问 WasmEdge 函数。`NativeLib` 源码 [可在此处获得](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/java/org/wasmedge/native_lib/NativeLib.kt)。它使用 JNI (Java 本地接口) 来加载名为 `wasmedge_lib` 的 C 共享库，然后调用 `wasmedge_lib` 中的 `nativeWasmFibonacci` 函数来执行 `fibonacci.wasm` WebAssembly 字节码。

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

### C 共享库

生成 C 共享库的源代码 `wasmedge_lib.cpp` [可在此处获取](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/cpp/wasmedge_lib.cpp)。它使用 WasmEdge C SDK 嵌入 WasmEdge VM 并执行 WebAssembly 函数。

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

### WebAssembly 函数

`factorial.wat` 是一个 [手写的 WebAssembly 脚本](https://github.com/WasmEdge/WasmEdge/blob/master/tools/wasmedge/examples/fibonacci.wat)，用于计算阶乘数。 使用 [WABT 工具](https://github.com/WebAssembly/wabt) 将 wat 编译成 WebAssembly。

### 构建依赖项

Android Studio 和 Gradle 使用 CMake 构建 C 共享库。[CMakeLists.txt 文件](https://github.com/WasmEdge/WasmEdge/blob/master/utils/android/app/lib/src/main/cpp/CMakeLists.txt) 将 WasmEdge 源代码构建为适用于 Android 的共享库文件，并将它们嵌入到最终的 APK 应用程序中。 在这种情况下，不需要执行单独的步骤将 WasmEdge 共享库安装到 Android 设备上。
