// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

/// This file contains tests of nested VM execution.
/// When entering compiled wasm functions current VM references are written to
/// thread local variables (e.g. `Executor::This`).
/// These references are read when compiled wasm calls host function.
/// There was a bug, when host function created nested VM to call another
/// compiled wasm function, and overwritten reference was not restored.

#include "wasmedge/wasmedge.h"

#include <array>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <stack>

namespace {
/// C++ overloaded error checking functions
/// `try` is keyword, `_try` is reserved in msvc
void _Try(const char *Name, const WasmEdge_Result &R) {
  if (not WasmEdge_ResultOK(R)) {
    throw std::runtime_error{
        fmt::format("{}: {}", Name, WasmEdge_ResultGetMessage(R))};
  }
}
template <typename T> T *_Try(const char *Name, T *R) {
  if (R == nullptr) {
    throw std::runtime_error{Name};
  }
  return R;
}
/// C++ error checking function wrapper.
/// Accepts same arguments `(A...)` as wrapped function instead of `(auto...)`.
template <typename T> struct TryWrap;
template <typename R, typename... A> struct TryWrap<R (*)(A...)> {
  static auto wrap(const char *Name, R (*Fn)(A...)) {
    return [Name, Fn](A... Args) { return _Try(Name, Fn(Args...)); };
  }
};
/// C++ error checking macro
#define TRY(fn) TryWrap<decltype(&fn)>::wrap(#fn, &fn)

/// C++ non-owned wasmedge string
struct StringView {
  WasmEdge_String Ffi{{}, {}};
  StringView() {}
  StringView(std::string_view S)
      : Ffi{WasmEdge_StringWrap(S.data(), static_cast<uint32_t>(S.size()))} {}
  operator WasmEdge_String() const { return Ffi; }
};

/// C++ overloaded wasmedge object deleters
void deletePtr(WasmEdge_LoaderContext *Ptr) { WasmEdge_LoaderDelete(Ptr); }
void deletePtr(WasmEdge_ASTModuleContext *Ptr) {
  WasmEdge_ASTModuleDelete(Ptr);
}
void deletePtr(WasmEdge_ValidatorContext *Ptr) {
  WasmEdge_ValidatorDelete(Ptr);
}
void deletePtr(WasmEdge_ExecutorContext *Ptr) { WasmEdge_ExecutorDelete(Ptr); }
void deletePtr(WasmEdge_StoreContext *Ptr) { WasmEdge_StoreDelete(Ptr); }
void deletePtr(WasmEdge_ModuleInstanceContext *Ptr) {
  WasmEdge_ModuleInstanceDelete(Ptr);
}
void deletePtr(WasmEdge_FunctionTypeContext *Ptr) {
  WasmEdge_FunctionTypeDelete(Ptr);
}
void deletePtr(WasmEdge_CompilerContext *Ptr) { WasmEdge_CompilerDelete(Ptr); }

/// C++ owned wasmedge object pointer
template <typename T> struct Ptr {
  using Pffi = T *;
  Pffi Ffi{};
  Ptr() {}
  Ptr(Pffi Ffi) : Ffi{Ffi} {}
  Ptr(const Ptr &) = delete;
  Ptr(Ptr &&Other) { std::swap(Ffi, Other.Ffi); }
  ~Ptr() {
    if (Ffi != nullptr) {
      deletePtr(Ffi);
    }
  }
  operator Pffi() const { return Ffi; }
  operator Pffi *() { return &Ffi; }
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
const std::array<uint8_t, 107> EmbeddedNestedWasm{
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
const char *PathNestedWasmCompiled = "aot_nested.wasm";

/// Forward declare host functions
void hostFnCheck(const WasmEdge_CallingFrameContext *Frame);
void hostFnOverwrite();

/// Compile embedded wasm
void compileWasm() {
  TRY(WasmEdge_CompilerCompileFromBuffer)
  (Ptr{TRY(WasmEdge_CompilerCreate)(nullptr)}, EmbeddedNestedWasm.data(),
   EmbeddedNestedWasm.size(), PathNestedWasmCompiled);
}

/// Stack of nested VM
thread_local std::stack<WasmEdge_ExecutorContext *> ExecutorsStack;
/// Call specified function from embedded wasm
void callWasm(const char *FnName) {
  Ptr<WasmEdge_ASTModuleContext> Module;
  TRY(WasmEdge_LoaderParseFromFile)
  (Ptr{TRY(WasmEdge_LoaderCreate)(nullptr)}, Module, PathNestedWasmCompiled);
  TRY(WasmEdge_ValidatorValidate)
  (Ptr{TRY(WasmEdge_ValidatorCreate)(nullptr)}, Module);

  static auto Executor = Ptr{TRY(WasmEdge_ExecutorCreate)(nullptr, nullptr)};
  auto Store = Ptr{TRY(WasmEdge_StoreCreate)()};
  auto Host = Ptr{TRY(WasmEdge_ModuleInstanceCreate)(StringView{"env"})};
  WasmEdge_ModuleInstanceAddFunction(
      Host, StringView{"hostFnCheck"},
      WasmEdge_FunctionInstanceCreate(
          Ptr{TRY(WasmEdge_FunctionTypeCreate)(nullptr, 0, nullptr, 0)},
          [](void *, const WasmEdge_CallingFrameContext *Frame,
             const WasmEdge_Value *, WasmEdge_Value *) {
            hostFnCheck(Frame);
            return WasmEdge_Result_Success;
          },
          nullptr, 0));
  WasmEdge_ModuleInstanceAddFunction(
      Host, StringView{"hostFnOverwrite"},
      WasmEdge_FunctionInstanceCreate(
          Ptr{TRY(WasmEdge_FunctionTypeCreate)(nullptr, 0, nullptr, 0)},
          [](void *, const WasmEdge_CallingFrameContext *,
             const WasmEdge_Value *, WasmEdge_Value *) {
            hostFnOverwrite();
            return WasmEdge_Result_Success;
          },
          nullptr, 0));
  TRY(WasmEdge_ExecutorRegisterImport)(Executor, Store, Host);

  Ptr<WasmEdge_ModuleInstanceContext> Instance;
  TRY(WasmEdge_ExecutorInstantiate)(Executor, Instance, Store, Module);
  auto *WasmFn =
      TRY(WasmEdge_ModuleInstanceFindFunction)(Instance, StringView{FnName});
  ExecutorsStack.push(Executor);
  TRY(WasmEdge_ExecutorInvoke)(Executor, WasmFn, nullptr, 0, nullptr, 0);
  ExecutorsStack.pop();
}

size_t CalledHostFnCheck = 0;
void hostFnCheck(const WasmEdge_CallingFrameContext *Frame) {
  ++CalledHostFnCheck;
  auto *Executor = WasmEdge_CallingFrameGetExecutor(Frame);
  EXPECT_EQ(Executor, ExecutorsStack.top());
}

size_t CalledHostFnOverwrite = 0;
void hostFnOverwrite() {
  ++CalledHostFnOverwrite;
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
  EXPECT_EQ(CalledHostFnCheck, 2);
  EXPECT_EQ(CalledHostFnOverwrite, 1);
}
} // namespace
