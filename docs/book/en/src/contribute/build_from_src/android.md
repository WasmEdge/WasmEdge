# Build WasmEdge for Android

The WasmEdge Runtime releases come with pre-built binaries for the Android OS. Why WasmEdge on Android?

* Native speed & sandbox safety for Android apps
* Support multiple dev languages — eg C, [Rust](../../write_wasm/rust.md), [Swift](../../write_wasm/swift.md), [Go](../../write_wasm/go.md) & [JS](../../write_wasm/js.md)
* [Embed 3rd party functions](../../sdk.md) in your android app
* [Kubernetes managed](../../use_cases/kubernetes.md) android apps

However, the WasmEdge installer does not support Android. The user must download the release files to a computer, and then use the `adb` tool to transfer the files to an Android device or simulator. We will show you how to do that.

* [WasmEdge CLI tools for Android](android/cli.md)
* [Call WasmEdge functions from an NDK native app](android/ndk.md)
* [Call WasmEdge functions from an Android APK app](android/apk.md)

## Build from source for Android platforms

Please follow this guide to build and test WasmEdge from source code with Android NDK.

> In current state, we only support the runtime for the interpreter mode.

## Prepare the Environment

We recommand developers to [use our Docker images](linux.md##prepare-the-environment) and follow the steps to prepare the building environment.

* Download and extract the [Android NDK 23b](https://developer.android.com/ndk/downloads).
* Check the cmake for [CMake 3.21](https://cmake.org/download/) or greater version.
* Download and install the [ADB platform tools](https://developer.android.com/studio/releases/platform-tools).
  * If you use the debian or ubuntu Linux distributions, you can install the ADB platform tools via `apt`.
* An Android device which is [enabled developer options and USB debugging](https://developer.android.com/studio/debug/dev-options) and with at least Android 6.0 or higher system version.

## Build WasmEdge for Android platforms

Get the WasmEdge source code.

```bash
git clone https://github.com/WasmEdge/WasmEdge.git
cd WasmEdge
```

Add the Android NDK path into the environment variable.

```bash
export ANDROID_NDK_HOME=path/to/you/ndk/dir
```

Run the build script in WasmEdge source code. This script will automatically build the WasmEdge for Android, and the results are in the `build` folder.

```bash
./utils/android/standalone/build_for_android.sh
```

## Test the WasmEdge CLI on Android platforms

### Push the WasmEdge CLI and related test data onto Android platforms

1. Connect the device by using a USB cable or Wi-Fi. Then you can check the attached devices via the `adb devices` command.

    ```bash
    $ adb devices
    List of devices attached
    0a388e93      device
    ```

2. Use the `adb push` command to push the entire `build/tools/wasmedge` folder into the `/data/local/tmp` folder on your Android device.

    ```bash
    cp -r examples build/tools/wasmedge/examples
    cd build
    adb push ./tools/wasmedge /data/local/tmp
    ```

### Run WasmEdge CLI on Android platforms

1. Please use the `adb shell` command to access into the Android device.
2. Follow the steps to test the WasmEdge CLI on the Android device.

```bash
$ cd /data/local/tmp/wasmedge/examples
$ ../wasmedge hello.wasm 1 2 3
hello
1
2
3

$ ../wasmedge --reactor add.wasm add 2 2
4

$ ../wasmedge --reactor fibonacci.wasm fib 8
34

$ ../wasmedge --reactor factorial.wasm fac 12
479001600

$ cd js
$ ./../wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3
```

## Notice

* For the Android 10 or greater versions, SELinux will disallow the untrusted applications' `exec()` system call to execute the binaries in `home` or `/data/local/tmp` folder.
* The Android SELinux policy will disallow the untrusted applications to access the `/data/local/tmp` folder.
