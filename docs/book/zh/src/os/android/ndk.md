# 从 NDK 原生应用调用 WasmEdge 函数

在本章节中，我们将演示如何使用 C 和 Android NDK 构建 Android 原生应用程序。原生应用使用 WasmEdge C SDK 嵌入 WasmEdge Runtime，通过 WasmEdge 调用 WASM 函数。

## 环境准备

### Android 开发者选项

目前 WasmEdge 仅支持 Android 平台的 arm64-v8a 架构，开发者需要准备 arm64-v8a 架构的 Android 模拟器或一台已经 [启用开发者选项和 USB 调试](https://developer.android.com/studio/debug/dev-options) 的 Android 设备便于测试，最低系统版本为 Android 6.0。

### Android 命令行工具

Ubuntu 环境下，开发者可以通过 `apt-get` 获取 Android 平台调试工具 `adb` 。通过在 Ubuntu 开发环境上执行 `adb shell` 指令，开发者可以进入设备并使用命令行操作 Android 操作系统。

```bash
sudo apt-get install adb
```

### Android NDK

T开发者需要下载并安装 [Android NDK](https://developer.android.google.cn/ndk/downloads) 以编译 C 语言源码为 Android 上可以运行的应用程序。在本示例中使用最新 Android NDK 发行版本 (r23b)。

## C 语言源码编程

[`test.c`](https://github.com/second-state/wasm-learning/blob/master/android/test.c) 使用 wasmedge-tensorflow c api 来运行 WebAssembly 函数。 WebAssembly 文件 `birds_v1.wasm` 是从 Rust 源代码编译而来的，[在此处查看解释](../../dev/rust/tensorflow.md)。

```c
#include <wasmedge/wasmedge.h>
#include <wasmedge/wasmedge-image.h>
#include <wasmedge/wasmedge-tensorflowlite.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
  /*
   * argv[0]: ./a.out
   * argv[1]: WASM file
   * argv[2]: tflite model file
   * argv[3]: image file
   * Usage: ./a.out birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
   */

  /* Create the VM context. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(ConfCxt, NULL);
  WasmEdge_ConfigureDelete(ConfCxt);
  
  /* Create the image and TFLite import objects. */
  WasmEdge_ImportObjectContext *ImageImpObj = WasmEdge_Image_ImportObjectCreate();
  WasmEdge_ImportObjectContext *TFLiteImpObj = WasmEdge_TensorflowLite_ImportObjectCreate();
  WasmEdge_ImportObjectContext *TFDummyImpObj = WasmEdge_Tensorflow_ImportObjectCreateDummy();

  /* Register into VM. */
  WasmEdge_VMRegisterModuleFromImport(VMCxt, ImageImpObj);
  WasmEdge_VMRegisterModuleFromImport(VMCxt, TFLiteImpObj);
  WasmEdge_VMRegisterModuleFromImport(VMCxt, TFDummyImpObj);

  /* Init WASI. */
  const char *Preopens[] = {".:."};
  const char *Args[] = {argv[1], argv[2], argv[3]};
  WasmEdge_ImportObjectContext *WASIImpObj = WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
  WasmEdge_ImportObjectInitWASI(WASIImpObj, Args, 3, NULL, 0, Preopens, 1);

  /* Run WASM file. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("_start");
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, argv[1], FuncName, NULL, 0, NULL, 0);
  WasmEdge_StringDelete(FuncName);

  /* Check the result. */
  if (!WasmEdge_ResultOK(Res)) {
    printf("Run WASM failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }

  WasmEdge_ImportObjectDelete(ImageImpObj);
  WasmEdge_ImportObjectDelete(TFLiteImpObj);
  WasmEdge_ImportObjectDelete(TFDummyImpObj);
  WasmEdge_VMDelete(VMCxt);
  return 0;
}
```

## 构建

### 安装依赖包

使用以下命令在您的 Ubuntu 开发环境上下载适用于 Android 的 WasmEdge。

```bash
wget https://github.com/WasmEdge/WasmEdge/releases/download/0.11.0/WasmEdge-0.11.0-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-image/releases/download/0.11.0/WasmEdge-image-0.11.0-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-tensorflow/releases/download/0.11.0/WasmEdge-tensorflowlite-0.11.0-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/0.11.0/WasmEdge-tensorflow-deps-TFLite-0.11.0-android_aarch64.tar.gz
tar -zxf WasmEdge-0.11.0-android_aarch64.tar.gz
tar -zxf WasmEdge-image-0.11.0-android_aarch64.tar.gz -C WasmEdge-0.11.0-Android/
tar -zxf WasmEdge-tensorflowlite-0.11.0-android_aarch64.tar.gz -C WasmEdge-0.11.0-Android/
tar -zxf WasmEdge-tensorflow-deps-TFLite-0.11.0-android_aarch64.tar.gz -C WasmEdge-0.11.0-Android/lib/
```

### 编译

使用以下命令在 Ubunu 开发环境上将 C 源码编译为 Android 上可以运行的程序 `a.out`。

```bash
(/path/to/ndk)/toolchains/llvm/prebuilt/(HostPlatform)/bin/aarch64-linux-(AndroidApiVersion)-clang test.c -I./WasmEdge-0.11.0-Android/include -L./WasmEdge-0.11.0-Android/lib -lwasmedge-image_c -lwasmedge-tensorflowlite_c -ltensorflowlite_c -lwasmedge
```

## 测试

### 推送文件到 Android 设备

在 Ubuntu 开发环境上使用 `adb` 命令将编译后的程序、Tensorflow Lite 模型文件、测试图像文件以及 Android 的 WasmEdge 共享库文件安装到 Android 设备上。

```bash
adb push a.out /data/local/tmp
adb push birds_v1.wasm /data/local/tmp
adb push lite-model_aiy_vision_classifier_birds_V1_3.tflite /data/local/tmp
adb push bird.jpg /data/local/tmp
adb push ./WasmEdge-0.11.0-Android/lib /data/local/tmp
```

### 运行示例

现在，在 Ubuntu 开发环境上运行 `adb shell` 后，您可以通过远程 shell 命令在 Android 设备上运行编译后的 C 程序。

```bash
$ adb shell
sirius:/ $ cd /data/local/tmp
sirius:/data/local/tmp $ export LD_LIBRARY_PATH=/data/local/tmp/lib:$LD_LIBRARY_PATH
sirius:/data/local/tmp $ ./a.out birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
INFO: Initialized TensorFlow Lite runtime.
166 : 0.84705883
```
