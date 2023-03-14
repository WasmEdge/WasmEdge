// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include <asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "bpf-api.h"
#include "common/types.h"
#include "wasmedge/wasmedge.h"

extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

using namespace std;
using namespace WasmEdge;

static int bpf_buffer_sample(void *ctx, void *data, size_t size);
static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
  if (level == LIBBPF_DEBUG && DEBUG_LIBBPF_RUNTIME)
    return 0;
  char buf[DEBUG_PRINT_BUFFER_SIZE];
  int len = vsnprintf(buf, sizeof(buf), format, args);
  spdlog::debug("{}", buf);
  return len;
}

/// \brief initialize libbpf library
void init_libbpf(void) {
  libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
  libbpf_set_print(libbpf_print_fn);
}

/// \brief perf buffer sample callback
static void perfbuf_sample_fn(void *ctx, int cpu, void *data, __u32 size) {
  (void)cpu;
  bpf_buffer_sample(ctx, data, size);
}

/// \brief sample the perf buffer and ring buffer
static int bpf_buffer_sample(void *ctx, void *data, size_t size) {
  bpf_buffer *buffer = static_cast<bpf_buffer *>(ctx);
  return buffer->bpf_buffer_sample(data, size);
}

#define PERF_BUFFER_PAGES 64

class perf_buffer_wrapper : public bpf_buffer {
  std::unique_ptr<perf_buffer, void (*)(perf_buffer *pb)> inner{
      nullptr, perf_buffer__free};

public:
  perf_buffer_wrapper(bpf_map *events) {
    bpf_map__set_type(events, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    bpf_map__set_key_size(events, sizeof(int));
    bpf_map__set_value_size(events, sizeof(int));
  }
  int bpf_buffer__poll(int timeout_ms) override {
    return perf_buffer__poll(inner.get(), timeout_ms);
  }
  int bpf_buffer__open(int fd, bpf_buffer_sample_fn sample_cb,
                       void *ctx) override {
    fn = sample_cb;
    inner.reset(perf_buffer__new(fd, PERF_BUFFER_PAGES, perfbuf_sample_fn, NULL,
                                 ctx, NULL));
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
  int bpf_buffer__poll(int timeout_ms) override {
    return ring_buffer__poll(inner.get(), timeout_ms);
  }
  int bpf_buffer__open(int fd, bpf_buffer_sample_fn sample_cb,
                       void *ctx) override {
    inner.reset(ring_buffer__new(fd, sample_cb, ctx, NULL));
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

int bpf_buffer::bpf_buffer_sample(void *data, size_t size) {
  size_t sample_size = size;
  if (max_poll_size < size) {
    sample_size = max_poll_size;
  }
  memcpy(poll_data, data, sample_size);
  auto module_inst = wasm_module_instance;
  WasmEdge_String names[10];
  uint32_t table_len __attribute__((unused)) = WasmEdge_ModuleInstanceListTable(
      module_inst, names, sizeof(names) / sizeof(names[0]));
  assert(table_len == 1);
  auto table_inst = WasmEdge_ModuleInstanceFindTable(module_inst, names[0]);
  assert(table_inst != nullptr);
  WasmEdge_Value value;
  {
    auto result __attribute__((unused)) =
        WasmEdge_TableInstanceGetData(table_inst, &value, wasm_sample_function);
    assert(WasmEdge_ResultOK(result));
  }
  assert(value.Type == WasmEdge_ValType::WasmEdge_ValType_FuncRef);
  auto func_ref = WasmEdge_ValueGetFuncRef(value);
  {
    WasmEdge_Value params[3] = {
        WasmEdge_ValueGenI32(wasm_ctx),
        WasmEdge_ValueGenI32(wasm_buf_ptr),
        WasmEdge_ValueGenI32(size),
    };
    WasmEdge_Value result[1];
    auto call_result __attribute__((unused)) = WasmEdge_ExecutorInvoke(
        wasm_executor, func_ref, params, sizeof(params) / sizeof(params[0]),
        result, sizeof(result) / sizeof(result[0]));
    assert(WasmEdge_ResultOK(call_result));
    return WasmEdge_ValueGetI32(result[0]);
  }
  return 0;
}

struct bpf_map *bpf_obj_get_map_by_fd(int fd, bpf_object *obj) {
  bpf_map *map;
  bpf_object__for_each_map(map, obj) {
    if (bpf_map__fd(map) == fd)
      return map;
  }
  return NULL;
}

/// \brief create a bpf buffer based on the object map type
std::unique_ptr<bpf_buffer> bpf_buffer__new(struct bpf_map *events) {
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
int wasm_bpf_program::bpf_map_fd_by_name(const char *name) {
  return bpf_object__find_map_fd_by_name(obj.get(), name);
}
/// @brief load all bpf programs and maps in a object file.
int wasm_bpf_program::load_bpf_object(const void *obj_buf, size_t obj_buf_sz) {
  auto object = bpf_object__open_mem(obj_buf, obj_buf_sz, NULL);
  if (!object) {
    return (int)libbpf_get_error(object);
  }
  obj.reset(object);
  return bpf_object__load(object);
}

/// @brief attach a specific bpf program by name and target.
int wasm_bpf_program::attach_bpf_program(const char *name,
                                         const char *attach_target) {
  struct bpf_link *link;
  if (!attach_target) {
    link =
        bpf_program__attach(bpf_object__find_program_by_name(obj.get(), name));
  } else {
    struct bpf_object *o = obj.get();
    struct bpf_program *prog = bpf_object__find_program_by_name(o, name);
    if (!prog) {
      spdlog::error("get prog %s fail", name);
      return -1;
    }
    // TODO: support more attach type libbpf cannot auto attach
  }
  if (!link) {
    return (int)libbpf_get_error(link);
  }
  links.emplace(std::unique_ptr<bpf_link, int (*)(bpf_link * obj)>{
      link, bpf_link__destroy});
  return 0;
}

/// polling the buffer, if the buffer is not created, create it.
int wasm_bpf_program::bpf_buffer_poll(
    WasmEdge_ExecutorContext *executor,
    const WasmEdge_ModuleInstanceContext *module_instance, int fd,
    int32_t sample_func, uint32_t ctx, void *data, size_t max_size,
    int timeout_ms, uint32_t wasm_buf_ptr) {
  int res;
  if (buffer.get() == nullptr) {
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
  buffer->set_callback_params(executor, module_instance, (uint32_t)sample_func,
                              data, max_size, ctx, wasm_buf_ptr);
  // poll the buffer
  res = buffer->bpf_buffer__poll(timeout_ms);
  if (res < 0) {
    return res;
  }
  return 0;
}
