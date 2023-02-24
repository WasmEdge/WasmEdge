# wasm-bpf Plugin

This plugin added six host functions that give you Wasm application access to eBPF.

Six functions are listed here. And all of them are in the module `wasm_bpf`, if you loaded this plugin.
```c
/// lookup a bpf map fd by name.
i32 wasm_bpf_map_fd_by_name(u64 obj, u32 name);
/// detach and close a bpf program.
i32 wasm_close_bpf_object(u64 obj);
/// CO-RE load a bpf object into the kernel.
u64 wasm_load_bpf_object(u32 obj_buf, u32 obj_buf_sz);
/// attach a bpf program to a kernel hook.
i32 wasm_attach_bpf_program(u64 obj, u32 name,
                            u32 attach_target);
/// poll a bpf buffer, and call a wasm callback indicated by sample_func.
/// the first time to call this function will open and create a bpf buffer.
i32 wasm_bpf_buffer_poll(u64 program, i32 fd, u32 sample_func,
                         u32 ctx, u32 data, i32 max_size,
                         i32 timeout_ms);
/// lookup, update, delete, and get_next_key operations on a bpf map.
i32 wasm_bpf_map_operate(u64 fd, i32 cmd, u32 key, u32 value,
                         u32 next_key, u64 flags);
```

- `iXX` denotes signed integer with `XX` bits
- `uXX` denotes unsigned integer with `XX` bits

## How to compile this plugin

### Install dependencies

#### Boost

Boost is requires to build `WasmEdge`. `wasm-bpf` itself doesn't require `boost`

Boost is optional. If you don't install it, CMake will download it for you.

For example, you can install boost with the following command on Ubuntu:
```
sudo apt install libboost-all-dev
```

#### libbpf

This plugin requires `libbpf >= 1.2`

Follow [https://github.com/libbpf/libbpf#building-libbpf](https://github.com/libbpf/libbpf#building-libbpf) to build and install `libbpf`.

### Build `wasm-bpf` plug-in

Run the following commands at the root of the `WasmEdge` project:

- Note: It's important to set `WASMEDGE_PLUGIN_WASM_BPF` to `TRUE` in the command line. This toggle controls the build of `wasm-bpf` plugin.

```
cmake -DWASMEDGE_PLUGIN_WASM_BPF:BOOL=TRUE -B ./build -G "Unix Makefiles"
cmake --build ./build
```

After building, you can find the plug-in `./build/plugins/wasm-bpf/libwasmedgePluginWasmBpf.so` and the WasmEdge CLI tool at `./build/tools/wasmedge/wasmedge`.

Set `WASMEDGE_PLUGIN_PATH=./build/plugins/wasm-bpf/` and run wasmedge:

```console
root@yutong-virtualbox:~/WasmEdge# WASMEDGE_PLUGIN_PATH=./build/plugins/wasm-bpf/ ./build/tools/wasmedge/wasmedge execve.wasm 

[289150] node -> /bin/sh -c which ps 
[289151] sh -> which ps 
[289152] node -> /bin/sh -c /usr/bin/ps -ax -o pid=,ppid=,pcpu=,pmem=,c 
[289153] sh -> /usr/bin/ps -ax -o pid=,ppid=,pcpu=,pmem=,command= 
[289154] node -> /bin/sh -c "/root/.vscode-server-insiders/bin/96a795cc 
[289155] sh -> /root/.vscode-server-insiders/bin/96a795cc0 245632 245678 289148 
[289156] cpuUsage.sh -> sed -n s/^cpu\s//p /proc/stat 
[289157] cpuUsage.sh -> cat /proc/245632/stat 
[289158] cpuUsage.sh -> cat /proc/245678/stat 
[289159] cpuUsage.sh -> cat /proc/289148/stat 
[289160] cpuUsage.sh -> sleep 1 
^C
```