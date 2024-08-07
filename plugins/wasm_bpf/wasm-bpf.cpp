// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include <asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "bpf-api.h"
#include "common/types.h"
#include "wasmedge/wasmedge.h"

extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

static int32_t bpf_buffer_sample(void *ctx, void *data, size_t size);
static int32_t libbpf_print_fn(enum libbpf_print_level level,
                               const char *format, va_list args) {
  if (level == LIBBPF_DEBUG && DEBUG_LIBBPF_RUNTIME)
    return 0;
  char buf[DEBUG_PRINT_BUFFER_SIZE];
  int32_t len = vsnprintf(buf, sizeof(buf), format, args);
  spdlog::debug("[WasmEdge Wasm_bpf] {}", buf);
  return len;
}

/// \brief perf buffer sample callback
static void perfbuf_sample_fn(void *ctx, int32_t cpu, void *data, __u32 size) {
  static_cast<void>(cpu);
  bpf_buffer_sample(ctx, data, size);
}

/// \brief sample the perf buffer and ring buffer
static int32_t bpf_buffer_sample(void *ctx, void *data, size_t size) {
  WasmEdge::Host::bpf_buffer *buffer =
      static_cast<WasmEdge::Host::bpf_buffer *>(ctx);
  return buffer->bpf_buffer_sample(data, size);
}

namespace WasmEdge {
namespace Host {

/// \brief initialize libbpf library
void init_libbpf(void) {
  libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
  libbpf_set_print(libbpf_print_fn);
}

class perf_buffer_wrapper : public bpf_buffer {
  std::unique_ptr<perf_buffer, void (*)(perf_buffer *pb)> inner{
      nullptr, perf_buffer__free};

public:
  perf_buffer_wrapper(bpf_map *events) {
    bpf_map__set_type(events, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    bpf_map__set_key_size(events, sizeof(int));
    bpf_map__set_value_size(events, sizeof(int));
  }
  int32_t bpf_buffer__poll(int32_t timeout_ms) override {
    return perf_buffer__poll(inner.get(), timeout_ms);
  }
  int32_t bpf_buffer__open(int32_t fd, bpf_buffer_sample_fn sample_cb,
                           void *ctx) override {
    fn = sample_cb;
    inner.reset(perf_buffer__new(fd, PERF_BUFFER_PAGES, perfbuf_sample_fn,
                                 nullptr, ctx, nullptr));
    return inner ? 0 : -EINVAL;
  }
};

struct ring_buffer_wrapper : public bpf_buffer {
public:
  std::unique_ptr<ring_buffer, void (*)(ring_buffer *pb)> inner{
      nullptr, ring_buffer__free};
  ring_buffer_wrapper(bpf_map *events) {
    bpf_map__set_autocreate(events, false);
  }
  int32_t bpf_buffer__poll(int32_t timeout_ms) override {
    return ring_buffer__poll(inner.get(), timeout_ms);
  }
  int32_t bpf_buffer__open(int32_t fd, bpf_buffer_sample_fn sample_cb,
                           void *ctx) override {
    inner.reset(ring_buffer__new(fd, sample_cb, ctx, nullptr));
    return inner ? 0 : -1;
  }
};

void bpf_buffer::set_callback_params(
    WasmEdge_ExecutorContext *executor,
    const WasmEdge_ModuleInstanceContext *module_instance, uint32_t sample_func,
    void *data, size_t max_size, uint32_t ctx, uint32_t buf_ptr) {
  wasm_executor = executor;
  wasm_module_instance = module_instance;
  wasm_sample_function = sample_func;
  poll_data = data;
  max_poll_size = max_size;
  wasm_ctx = ctx;
  wasm_buf_ptr = buf_ptr;
}

bool bpf_buffer::is_valid() const {
  auto module_inst = wasm_module_instance;
  WasmEdge_String names;
  uint32_t exported_table_len =
      WasmEdge_ModuleInstanceListTable(module_inst, &names, 1);
  if (exported_table_len != 1) {
    return false;
  }
  auto table_inst = WasmEdge_ModuleInstanceFindTable(module_inst, names);
  if (!table_inst) {
    return false;
  }
  WasmEdge_Value value;
  auto get_data_result =
      WasmEdge_TableInstanceGetData(table_inst, &value, wasm_sample_function);
  return WasmEdge_ResultOK(get_data_result);
}

int32_t bpf_buffer::bpf_buffer_sample(void *data, size_t size) {
  size_t sample_size = size;
  if (max_poll_size < size) {
    sample_size = max_poll_size;
  }
  memcpy(poll_data, data, sample_size);
  auto module_inst = wasm_module_instance;
  WasmEdge_String names[1];
  /// a valid module instance should have only one table
  uint32_t exported_table_len =
      WasmEdge_ModuleInstanceListTable(module_inst, names, std::size(names));
  assuming(exported_table_len == 1);
  auto table_inst = WasmEdge_ModuleInstanceFindTable(module_inst, names[0]);
  assuming(table_inst);
  WasmEdge_Value value;
  auto get_data_result =
      WasmEdge_TableInstanceGetData(table_inst, &value, wasm_sample_function);
  assuming(WasmEdge_ResultOK(get_data_result));
  assert(value.Type == WasmEdge_ValType::WasmEdge_ValType_FuncRef);
  auto func_ref = WasmEdge_ValueGetFuncRef(value);

  WasmEdge_Value invoke_func_params[3] = {
      WasmEdge_ValueGenI32(wasm_ctx),
      WasmEdge_ValueGenI32(wasm_buf_ptr),
      WasmEdge_ValueGenI32(size),
  };
  WasmEdge_Value invoke_func_result;
  auto call_result = WasmEdge_ExecutorInvoke(
      wasm_executor, func_ref, invoke_func_params, 3, &invoke_func_result, 1);
  if (!WasmEdge_ResultOK(call_result)) {
    return -EINVAL;
  }
  return WasmEdge_ValueGetI32(invoke_func_result);
}

/// \brief create a bpf buffer based on the object map type
std::unique_ptr<bpf_buffer> bpf_buffer__new(bpf_map *events) {
  bpf_map_type map_type = bpf_map__type(events);
  switch (map_type) {
  case BPF_MAP_TYPE_PERF_EVENT_ARRAY:
    return std::make_unique<perf_buffer_wrapper>(events);
  case BPF_MAP_TYPE_RINGBUF:
    return std::make_unique<ring_buffer_wrapper>(events);
  default:
    return nullptr;
  }
}

/// Get the file descriptor of a map by name.
int32_t wasm_bpf_program::bpf_map_fd_by_name(const char *name) {
  return bpf_object__find_map_fd_by_name(obj.get(), name);
}
/// \brief load all bpf programs and maps in a object file.
int32_t wasm_bpf_program::load_bpf_object(const void *obj_buf,
                                          size_t obj_buf_sz) {
  auto object = bpf_object__open_mem(obj_buf, obj_buf_sz, nullptr);
  if (!object) {
    return static_cast<int32_t>(libbpf_get_error(object));
  }
  obj.reset(object);
  return bpf_object__load(object);
}

/// \brief attach a specific bpf program by name and target.
int32_t wasm_bpf_program::attach_bpf_program(const char *name,
                                             const char *attach_target) {
  bpf_link *link;
  if (!attach_target) {
    // auto attach base on bpf_program__section_name. The works well for most
    // bpf types, include kprobe, uprobe, fentry, lsm, etc.
    link =
        bpf_program__attach(bpf_object__find_program_by_name(obj.get(), name));
  } else {
    bpf_object *o = obj.get();
    bpf_program *prog = bpf_object__find_program_by_name(o, name);
    if (!prog) {
      spdlog::error("[WasmEdge Wasm_bpf] get prog {} fail", name);
      return -1;
    }
    // TODO: attach dynamically base on bpf_program__section_name(prog) and
    // attach_target to support more attach type libbpf cannot auto attach. For
    // example, if bpf_program__section_name(prog) is "xdp" and attach_target is
    // "eth0", or attach sockops to a socket fd. For now, we will try auto
    // attach as well.
    link =
        bpf_program__attach(bpf_object__find_program_by_name(obj.get(), name));
  }
  if (!link) {
    return static_cast<int32_t>(libbpf_get_error(link));
  }
  links.emplace(std::unique_ptr<bpf_link, int32_t (*)(bpf_link * obj)>{
      link, bpf_link__destroy});
  return 0;
}

/// \brief get map pointer by fd through iterating over all maps
bpf_map *wasm_bpf_program::map_ptr_by_fd(int fd) {
  bpf_map *curr = nullptr;
  bpf_map__for_each(curr, obj.get()) {
    if (bpf_map__fd(curr) == fd) {
      return curr;
    }
  }
  return nullptr;
}

/// polling the buffer, if the buffer is not created, create it.
int32_t wasm_bpf_program::bpf_buffer_poll(
    WasmEdge_ExecutorContext *executor,
    const WasmEdge_ModuleInstanceContext *module_instance, int32_t fd,
    int32_t sample_func, uint32_t ctx, void *data, size_t max_size,
    int32_t timeout_ms, uint32_t wasm_buf_ptr) {
  int32_t res;
  if (!buffer.get()) {
    // create buffer
    auto map = map_ptr_by_fd(fd);
    buffer = bpf_buffer__new(map);
    if (!buffer) {
      return -1;
    }
    res = buffer->bpf_buffer__open(fd, bpf_buffer_sample, buffer.get());
    if (res < 0) {
      return res;
    }
  }
  buffer->set_callback_params(executor, module_instance,
                              static_cast<uint32_t>(sample_func), data,
                              max_size, ctx, wasm_buf_ptr);
  if (!buffer->is_valid()) {
    return -EINVAL;
  }
  // poll the buffer
  return buffer->bpf_buffer__poll(timeout_ms);
}

} // namespace Host
} // namespace WasmEdge
