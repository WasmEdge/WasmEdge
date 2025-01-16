// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

/// This file contains tests of nested VM execution.
/// When entering compiled wasm functions current VM references are written to
/// thread local variables (e.g. `Executor::This`).
/// These references are read when compiled wasm calls host function.
/// There was a bug, when host function created nested VM to call another
/// compiled wasm function, and overwritten reference was not restored.

#include "wasmedge/wasmedge.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <stack>

/// C++ overloaded error checking functions
/// `try` is keyword, `_try` is reserved in msvc
void _Try(const char *name, const WasmEdge_Result &r) {
  if (not WasmEdge_ResultOK(r)) {
    throw std::runtime_error{
        fmt::format("{}: {}", name, WasmEdge_ResultGetMessage(r))};
  }
}
template <typename T> T *_Try(const char *name, T *r) {
  if (r == nullptr) {
    throw std::runtime_error{name};
  }
  return r;
}
/// C++ error checking macro
#define TRY(fn)                                                                \
  [](auto &&...args) {                                                         \
    return _Try(#fn, fn(std::forward<decltype(args)>(args)...));               \
  }

/// C++ non-owned wasmedge string
struct StringView {
  WasmEdge_String ffi{{}, {}};
  StringView() {}
  StringView(std::string_view s)
      : ffi{WasmEdge_StringWrap(s.data(), s.size())} {}
  operator WasmEdge_String() const { return ffi; }
};

/// C++ overloaded wasmedge object deleters
void deletePtr(WasmEdge_LoaderContext *ptr) { WasmEdge_LoaderDelete(ptr); }
void deletePtr(WasmEdge_ASTModuleContext *ptr) {
  WasmEdge_ASTModuleDelete(ptr);
}
void deletePtr(WasmEdge_ValidatorContext *ptr) {
  WasmEdge_ValidatorDelete(ptr);
}
void deletePtr(WasmEdge_ExecutorContext *ptr) { WasmEdge_ExecutorDelete(ptr); }
void deletePtr(WasmEdge_StoreContext *ptr) { WasmEdge_StoreDelete(ptr); }
void deletePtr(WasmEdge_ModuleInstanceContext *ptr) {
  WasmEdge_ModuleInstanceDelete(ptr);
}
void deletePtr(WasmEdge_FunctionTypeContext *ptr) {
  WasmEdge_FunctionTypeDelete(ptr);
}
void deletePtr(WasmEdge_CompilerContext *ptr) { WasmEdge_CompilerDelete(ptr); }

/// C++ owned wasmedge object pointer
template <typename T> struct Ptr {
  using Ffi = T *;
  Ffi ffi{};
  Ptr() {}
  Ptr(Ffi ffi) : ffi{ffi} {}
  Ptr(const Ptr &) = delete;
  Ptr(Ptr &&other) { std::swap(ffi, other.ffi); }
  ~Ptr() {
    if (ffi != nullptr) {
      deletePtr(ffi);
    }
  }
  operator Ffi() const { return ffi; }
  operator Ffi *() { return &ffi; }
};

/*
(module
  (import "env" "hostFnCheck" (func $hostFnCheck))
  (import "env" "hostFnOverwrite" (func $hostFnOverwrite))
  (func (export "wasmFnOverwrite")
  )
  (func (export "wasmFn")
    call $hostFnCheck
    call $hostFnOverwrite
    call $hostFnCheck
  )
)
*/
const std::array<uint8_t, 107> embedded_nested_wasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x29, 0x02, 0x03, 0x65, 0x6e, 0x76, 0x0b, 0x68, 0x6f,
    0x73, 0x74, 0x46, 0x6e, 0x43, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x00, 0x03,
    0x65, 0x6e, 0x76, 0x0f, 0x68, 0x6f, 0x73, 0x74, 0x46, 0x6e, 0x4f, 0x76,
    0x65, 0x72, 0x77, 0x72, 0x69, 0x74, 0x65, 0x00, 0x00, 0x03, 0x03, 0x02,
    0x00, 0x00, 0x07, 0x1c, 0x02, 0x0f, 0x77, 0x61, 0x73, 0x6d, 0x46, 0x6e,
    0x4f, 0x76, 0x65, 0x72, 0x77, 0x72, 0x69, 0x74, 0x65, 0x00, 0x02, 0x06,
    0x77, 0x61, 0x73, 0x6d, 0x46, 0x6e, 0x00, 0x03, 0x0a, 0x0d, 0x02, 0x02,
    0x00, 0x0b, 0x08, 0x00, 0x10, 0x00, 0x10, 0x01, 0x10, 0x00, 0x0b,
};

/// Test compiles embedded wasm to this file
const char *path_nested_wasm_compiled = "aot_nested.wasm";

/// Forward declare host functions
void hostFnCheck(const WasmEdge_CallingFrameContext *frame);
void hostFnOverwrite();

/// Compile embedded wasm
void compileWasm() {
  TRY(WasmEdge_CompilerCompileFromBuffer)
  (Ptr{TRY(WasmEdge_CompilerCreate)(nullptr)}, embedded_nested_wasm.data(),
   embedded_nested_wasm.size(), path_nested_wasm_compiled);
}

/// Stack of nested VM
thread_local std::stack<WasmEdge_ExecutorContext *> executors_stack;
/// Call specified function from embedded wasm
void callWasm(const char *fn_name) {
  Ptr<WasmEdge_ASTModuleContext> module;
  TRY(WasmEdge_LoaderParseFromFile)
  (Ptr{TRY(WasmEdge_LoaderCreate)(nullptr)}, module, path_nested_wasm_compiled);
  TRY(WasmEdge_ValidatorValidate)
  (Ptr{TRY(WasmEdge_ValidatorCreate)(nullptr)}, module);

  static auto executor = Ptr{TRY(WasmEdge_ExecutorCreate)(nullptr, nullptr)};
  auto store = Ptr{TRY(WasmEdge_StoreCreate)()};
  auto host = Ptr{TRY(WasmEdge_ModuleInstanceCreate)(StringView{"env"})};
  WasmEdge_ModuleInstanceAddFunction(
      host, StringView{"hostFnCheck"},
      WasmEdge_FunctionInstanceCreate(
          TRY(WasmEdge_FunctionTypeCreate)(nullptr, 0, nullptr, 0),
          [](void *, const WasmEdge_CallingFrameContext *frame,
             const WasmEdge_Value *, WasmEdge_Value *) {
            hostFnCheck(frame);
            return WasmEdge_Result_Success;
          },
          nullptr, 0));
  WasmEdge_ModuleInstanceAddFunction(
      host, StringView{"hostFnOverwrite"},
      WasmEdge_FunctionInstanceCreate(
          TRY(WasmEdge_FunctionTypeCreate)(nullptr, 0, nullptr, 0),
          [](void *, const WasmEdge_CallingFrameContext *,
             const WasmEdge_Value *, WasmEdge_Value *) {
            hostFnOverwrite();
            return WasmEdge_Result_Success;
          },
          nullptr, 0));
  TRY(WasmEdge_ExecutorRegisterImport)(executor, store, host);

  Ptr<WasmEdge_ModuleInstanceContext> instance;
  TRY(WasmEdge_ExecutorInstantiate)(executor, instance, store, module);
  auto *wasm_fn =
      TRY(WasmEdge_ModuleInstanceFindFunction)(instance, StringView{fn_name});
  executors_stack.push(executor);
  TRY(WasmEdge_ExecutorInvoke)(executor, wasm_fn, nullptr, 0, nullptr, 0);
  executors_stack.pop();
}

size_t called_hostFnCheck = 0;
void hostFnCheck(const WasmEdge_CallingFrameContext *frame) {
  ++called_hostFnCheck;
  auto *executor = WasmEdge_CallingFrameGetExecutor(frame);
  EXPECT_EQ(executor, executors_stack.top());
}

size_t called_hostFnOverwrite = 0;
void hostFnOverwrite() {
  ++called_hostFnOverwrite;
  callWasm("wasmFnOverwrite");
}

/*
Call graph:
  wasmFn()               // Executor::This = executor1
    hostFnCheck()        // EXPECT_EQ(Executor::This, executor1)
    hostFnOverwrite()    // create nested VM
      wasmFnOverwrite()  // Executor::This = executor2
    hostFnCheck()        // EXPECT_EQ(Executor::This, executor1)
*/
TEST(APIAOTNestedVMTest, NestedVM) {
  compileWasm();
  callWasm("wasmFn");
  // wasmedge doesn't build GMock by default
  EXPECT_EQ(called_hostFnCheck, 2);
  EXPECT_EQ(called_hostFnOverwrite, 1);
}
