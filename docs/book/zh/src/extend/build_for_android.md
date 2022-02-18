# 为 Android 编译

## 使用 Android NDK 构建和测试 WasmEdge 命令行工具

此教程指导使用 Android NDK 构建 Interpreter 模式的 WasmEdge ，并在 Android 设备中测试 wasmedge 命令行工具。

### 环境准备

* [Android NDK 23b](https://developer.android.com/ndk/downloads)
  * 下载到的 NDK 工具链需要解压到磁盘上
* [CMake 3.21](https://cmake.org/download/) 及以上版本
  * 如果本机已安装 cmake，请检查版本是否符合要求
* [ADB](https://developer.android.com/studio/releases/platform-tools)
  * 将下载到的 platform-tools 解压到磁盘上，adb 命令在 bin 目录中
  * 如果您使用的是 debian/ubuntu 系统，可以通过 apt 安装 adb
* 一台已经 [启用开发者选项和 USB 调试](https://developer.android.com/studio/debug/dev-options) 的 Android 设备, 最低系统版本为 Android 6.0

### 构建 WasmEdge

1. 添加 NDK 目录路径到环境变量 `ANDROID_NDK_HOME=path/to/you/ndk/dir`
2. 执行 WasmEdge 源码路径下的 `utils/android/standalone/build_for_android.sh` 命令行脚本，将自动执行构建，构建结果在 WasmEdge 目录中的 build 目录

### 测试

#### 推送到 Android 设备

1. 将 Android 设备通过 USB 或 WLAN 连接到 PC 。您可以通过 `adb devices` 命令检查已连接的设备，可以获得类似如下显示：

    ```bash
    $ adb devices
    List of devices attached
    0a388e93      device
    ```

2. 使用 `adb push`命令推送 build/tools/wasmedge 到 Android 设备的 /data/local/tmp 目录

    ```bash
    cd build
    adb push ./tools/wasmedge /data/local/tmp  
    ```

#### 在 Android 设备中执行 WasmEdge

1. 使用 `adb shell` 命令进入 Android 设备
2. 测试运行 wasmedge 程序

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

### 注意

* Android 10 及以上系统版本，SELinux 限制普通 Android 应用程序使用 exec() 执行 home 目录中的可执行文件。[参考](https://android.googlesource.com/platform/system/sepolicy/+/08450264ae3f917f6b8e4091d6fedf84ef8d796f/private/untrusted_app_all.te#27)
* Android SELinux 限制普通 Android 应用程序访问 /data/local/tmp 目录
