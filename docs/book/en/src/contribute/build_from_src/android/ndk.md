# Call WasmEdge functions from an NDK native app

In this section, we will demonstrate how to build an Android native application using C and the Android SDK. The native application uses the WasmEdge C SDK to embed the WasmEdge Runtime, and call WASM functions through WasmEdge.

## Prerequisite

### Android

Currently, WasmEdge only supports the arm64-v8a architecture on Android devices. You need an arm64-v8a Android simulator or a physical device with [developer options turned on](https://developer.android.com/studio/debug/dev-options). WasmEdge requires Android 6.0 and above.

### Android development CLI

In Ubuntu Linux, you can use the `apt-get` command to install Android debugging and testing tool `adb`. Using the `adb shell` command on the Ubuntu dev machine, you can open a CLI shell to execute commands on the connected Android device.

```bash
sudo apt-get install adb
```

### Android NDK

To compile programs with the wasmedge-tensorflow c api, you need to install the [Android NDK](https://developer.android.google.cn/ndk/downloads). In this example, we use the latest LTS version (r23b).

## Review of source code

The [`test.c`](https://github.com/second-state/wasm-learning/blob/master/android/test.c) uses the wasmedge-tensorflow c api to run a WebAssembly function. The WebAssembly file `birds_v1.wasm` is compiled from Rust source code and [explained here](../../../write_wasm/rust/tensorflow.md).

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
  WasmEdge_ModuleInstanceContext *ImageImpObj = WasmEdge_Image_ModuleInstanceCreate();
  WasmEdge_ModuleInstanceContext *TFLiteImpObj = WasmEdge_TensorflowLite_ModuleInstanceCreate();
  WasmEdge_ModuleInstanceContext *TFDummyImpObj = WasmEdge_Tensorflow_ModuleInstanceCreateDummy();

  /* Register into VM. */
  WasmEdge_VMRegisterModuleFromImport(VMCxt, ImageImpObj);
  WasmEdge_VMRegisterModuleFromImport(VMCxt, TFLiteImpObj);
  WasmEdge_VMRegisterModuleFromImport(VMCxt, TFDummyImpObj);

  /* Init WASI. */
  const char *Preopens[] = {".:."};
  const char *Args[] = {argv[1], argv[2], argv[3]};
  WasmEdge_ModuleInstanceContext *WASIImpObj = WasmEdge_VMGetImportModuleContext(VMCxt, WasmEdge_HostRegistration_Wasi);
  WasmEdge_ModuleInstanceInitWASI(WASIImpObj, Args, 3, NULL, 0, Preopens, 1);

  /* Run WASM file. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("_start");
  WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VMCxt, argv[1], FuncName, NULL, 0, NULL, 0);
  WasmEdge_StringDelete(FuncName);

  /* Check the result. */
  if (!WasmEdge_ResultOK(Res)) {
    printf("Run WASM failed: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }

  WasmEdge_ModuleInstanceDelete(ImageImpObj);
  WasmEdge_ModuleInstanceDelete(TFLiteImpObj);
  WasmEdge_ModuleInstanceDelete(TFDummyImpObj);
  WasmEdge_VMDelete(VMCxt);
  return 0;
}
```

## Build

### Install dependencies

Use the following commands to download WasmEdge for Android on your Ubuntu dev machine.

```bash
wget https://github.com/WasmEdge/WasmEdge/releases/download/{{ wasmedge_version }}/WasmEdge-{{ wasmedge_version }}-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-image/releases/download/{{ wasmedge_version }}/WasmEdge-image-{{ wasmedge_version }}-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-tensorflow/releases/download/{{ wasmedge_version }}/WasmEdge-tensorflowlite-{{ wasmedge_version }}-android_aarch64.tar.gz
wget https://github.com/second-state/WasmEdge-tensorflow-deps/releases/download/{{ wasmedge_version }}/WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-android_aarch64.tar.gz
tar -zxf WasmEdge-{{ wasmedge_version }}-android_aarch64.tar.gz
tar -zxf WasmEdge-image-{{ wasmedge_version }}-android_aarch64.tar.gz -C WasmEdge-{{ wasmedge_version }}-Android/
tar -zxf WasmEdge-tensorflowlite-{{ wasmedge_version }}-android_aarch64.tar.gz -C WasmEdge-{{ wasmedge_version }}-Android/
tar -zxf WasmEdge-tensorflow-deps-TFLite-{{ wasmedge_version }}-android_aarch64.tar.gz -C WasmEdge-{{ wasmedge_version }}-Android/lib/
```

### Compile

The following command compiles the C program to `a.out` on your Ubunu dev machine.

```bash
(/path/to/ndk)/toolchains/llvm/prebuilt/(HostPlatform)/bin/aarch64-linux-(AndroidApiVersion)-clang test.c -I./WasmEdge-{{ wasmedge_version }}-Android/include -L./WasmEdge-{{ wasmedge_version }}-Android/lib -lwasmedge-image_c -lwasmedge-tensorflowlite_c -ltensorflowlite_c -lwasmedge
```

## Run

### Push files onto Android

Install the compiled program, Tensorflow Lite model file, test image file, as well as WasmEdge shared library files for Android, onto the Android device using `adb` from your Ubuntu dev machine.

```bash
adb push a.out /data/local/tmp
adb push birds_v1.wasm /data/local/tmp
adb push lite-model_aiy_vision_classifier_birds_V1_3.tflite /data/local/tmp
adb push bird.jpg /data/local/tmp
adb push ./WasmEdge-{{ wasmedge_version }}-Android/lib /data/local/tmp
```

### Run the example

Now you can run the compiled C program on the Android device via a remote shell command. Run `adb shell` from your Ubuntu dev machine.

```bash
$ adb shell
sirius:/ $ cd /data/local/tmp
sirius:/data/local/tmp $ export LD_LIBRARY_PATH=/data/local/tmp/lib:$LD_LIBRARY_PATH
sirius:/data/local/tmp $ ./a.out birds_v1.wasm lite-model_aiy_vision_classifier_birds_V1_3.tflite bird.jpg
INFO: Initialized TensorFlow Lite runtime.
166 : 0.84705883
```
