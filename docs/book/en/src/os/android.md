# Android

The WasmEdge Runtime releases come with pre-built binaries for the Android OS. Why WasmEdge on Android?

* Native speed & sandbox safety for Android apps
* Support multiple dev languages — eg C, [Rust](../dev/rust.md), [Swift](../dev/swift.md), [Go](../dev/go.md) & [JS](../dev/js.md)
* [Embed 3rd party functions](../embed.md) in your android app
* [Kubernetes managed](../kubernetes.md) android apps

However, the WasmEdge installer does not support Android. The user must download the release files to a computer, and then use the `adb` tool to transfer the files to an Android device or simulator. We will show you how to do that.

> If you prefer to build WasmEdge Runtime binary on Android for yourself, you can refer to the [build WasmEdge for Android](../extend/build_for_android.md) document.

* [WasmEdge CLI tools for Android](android/cli.md)
* [Call WasmEdge functions from an NDK native app](android/ndk.md)
* [Call WasmEdge functions from an Android APK app](android/studio.md)
