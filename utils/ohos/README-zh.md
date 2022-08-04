# OpenHarmony

## 介绍

WebAssembly 或者 wasm 作为一种可移植、体积小、加载快并且兼容 Web 的全新格式，运行在一个沙箱化的执行环境中，具有高效，安全等特点，现如今越来越多地被使用在云原生应用程序中。WasmEdge 是一种轻量级、高性能和可扩展的 WebAssembly runtime，适用于云原生、边缘和去中心化应用程序。它可以运行从 C/C++、Rust、Swift、AssemblyScript 或 Kotlin 源代码编译的标准 WebAssembly 字节码程序。

在 OpenHarmony 中支持 WasmEdge ，一方面，完善了 WasmEdge 的跨平台支持， 使 WasmEdge 的生态系统更加健壮； 另一方面，WasmEdge 提供给 需要在 OpenHarmony 上使用 Wasm 进行开发的用户一个温床， WasmEdge 的高性能更给 OHOS 的开发者们提供了新的选择：通过 WasmEdge，开发者仅需要付出较小的代价就能访问系统底层应用，而执行效率几乎达到原生效率。

## 从源码在 OpenHarmony Standard 上构建和测试 WasmEdge

请参考这个教程来从源码在 OpenHarmony 标准系统中构建和测试 WasmEdge

### 环境准备

#### OpenHarmony 标准系统

OpenHarmony 标准系统为开发者提供的 Docker 环境将对应的编译工具链进行了封装，本文档主要介绍在 Docker 环境下构建 WasmEdge 的步骤.

OpenHarmony 源码的获取与编译可以参考文档 [搭建Ubuntu环境-Docker方式](https://www.openharmony.cn/pages/00010102/)

请注意，在构建 WasmEdge 前需要将 Openharmony 进行一次全量编译以便后续WasmEdge的交叉编译过程.

```bash
# 获取到 docker 镜像后
$ docker run -it -v $(pwd):/home/openharmony openharmony-docker-standard:0.0.5
(root@xxxxxx:/home/openharmony/)$ ./build.sh --product-name Hi3516DV300
```

### 获取源码

OpenHarmony 将三方库项目放在了 third_party 文件夹下，因此本文档在 third_party 文件夹下获取 WasmEdge 源码，用户可以根据需要更改路径并修改相关配置文件中的路径。

```bash
$ docker run -it -v $(pwd):/home/openharmony openharmony-docker-standard:0.0.5
(root@xxxxxx:/home/openharmony)$ cd third_party
(root@xxxxxx:/home/openharmony/third_party)$ git clone https://github.com/WasmEdge/WasmEdge.git
(root@xxxxxx:/home/openharmony/third_party)$ cd WasmEdge
(root@xxxxxx:/home/openharmony/third_party/WasmEdge)$ 
```

### 修改 OpenHarmony 标准系统配置文件

#### 添加 WasmEdge 子系统配置

修改 OpenHarmony 的 build 目录下的 `subsystem_config.json` 文件，添加 `wasmedge` 子系统。

```json
{
  ...
  
  "wasmedge": {
    "path": "third_party/WasmEdge",
    "name": "wasmedge"
  },
  
  ...
}
```

#### 将组件添加到产品配置中

修改 OpenHarmony 产品配置文件，标准系统对应的配置文件：`productdefine/common/products/Hi3516DV300.json`。
在该配置文件中添加 `"wasmedge:wasmedge":{}`，表示该产品中会编译并打包 wasmedge 子系统下的 `wasmedge` 模块到版本中。

```json
{
  ...
  "parts":{
    ...
    "wasmedge:wasmedge":{}
  }
}
```

### 构建 WasmEdge

#### 说明

在 OpenHarmony 中构建的 WasmEdge 目前仅支持 `wasmedge`，即 wasm 的通用运行时。

* `wasmedge` 可以在解释器模式下执行一个 `WASM` 文件， 也可以在预编译模式下执行通用 Wasm 二进制格式文件， 目前还不支持在 OpenHarmony 中使用预编译模式 。

#### 执行构建脚本

通过执行 WasmEdge 源码下的 `utils/build_for_ohos.sh` 命令行脚本，将自动执行以下工作：

1. 将 .gn 等 OpenHarmony 需要的构建配置文件移动到 WasmEdge 项目根目录。
2. 使用 OpenHarmony 的编译工具链进行交叉编译构建 WasmEdge。
3. 运行 OpenHarmony 的构建脚本 `build.sh` 进行全量编译，该步骤将 `wasmedge` 添加进 OpenHarmony OS。

```bash
$ docker run -it -v $(pwd):/home/openharmony openharmony-docker-standard:0.0.5
(root@xxxxxx:/home/openharmony)$ cd third_party/WasmEdge/utils/ohos
(root@xxxxxx:/home/openharmony/third_party/WasmEdge/utils/ohos)$ ./build_for_ohos.sh /home/openharmony
```

当显示以下信息时，表明编译完成.

```bash
...

post_process
=====build Hi3516DV300 successful.
2021-12-15 03:18:50
++++++++++++++++++++++++++++++++++++++++

```

检查 `wasmedge` 是否编译打包进 OpenHarmony OS。

```bash
(root@xxxxxx:/home/openharmony/third_party/WasmEdge/utils/ohos)$ cd /home/openharmony/out/ohos-arm-release/packages/phone/system/bin
(root@xxxxxx:/home/openharmony/out/ohos-arm-release/packages/phone/system/bin)$ ls 
```

当输出的文件名中存在 `wasmedge` ，表明成功加入 OpenHarmony OS。

### 测试

#### 烧录镜像

将重新编译后的 OpenHarmony 标准系统镜像烧录进开发板，具体见 OpenHarmony 文档[Hi3516DV300开发板烧录](https://device.harmonyos.com/cn/docs/documentation/guide/hi3516_upload-0000001052148681)

#### 运行应用

在 OpenHarmony 标准系统中，WasmEdge 提供了测试样例，并写入了 system 镜像中，供用户进行测试。
通过串口工具连接上开发板并启动OpenHarmony标准系统后，用户可以进行以下测试。

```bash

$ cd /system/usr/wasmedge_example
$ wasmedge hello.wasm 1 2 3
hello
1
2
3
$ wasmedge --reactor add.wasm add 2 2
4
$ wasmedge --reactor fibonacci.wasm fib 8
34
$ wasmedge --reactor factorial.wasm fac 12
479001600
$ cd js
$ wasmedge --dir .:. qjs.wasm hello.js 1 2 3
Hello 1 2 3

```

### 开发

接下来，你可以使用 WasmEdge Runtime 在 OpenHarmony 标准系统中进行 WebAssembly 的相关开发工作。
