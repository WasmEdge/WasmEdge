// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// Ported gtest versions of the spec wasm-c-api example programs at
// https://github.com/WebAssembly/wasm-c-api/tree/main/example.

#include "wasm.h"
#include "wasm.hh"

#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {

// wasm.h's WASM_*_VAL macros use C99-style designated initializers, which
// MSVC rejects under /std:c++17. Inline helpers do the same construction
// in plain field assignment.
[[maybe_unused]] inline wasm_val_t initVal() {
  wasm_val_t v{};
  v.kind = WASM_EXTERNREF;
  v.of.ref = nullptr;
  return v;
}
[[maybe_unused]] inline wasm_val_t i32Val(int32_t i) {
  wasm_val_t v{};
  v.kind = WASM_I32;
  v.of.i32 = i;
  return v;
}
[[maybe_unused]] inline wasm_val_t i64Val(int64_t i) {
  wasm_val_t v{};
  v.kind = WASM_I64;
  v.of.i64 = i;
  return v;
}
[[maybe_unused]] inline wasm_val_t f32Val(float f) {
  wasm_val_t v{};
  v.kind = WASM_F32;
  v.of.f32 = f;
  return v;
}
[[maybe_unused]] inline wasm_val_t f64Val(double f) {
  wasm_val_t v{};
  v.kind = WASM_F64;
  v.of.f64 = f;
  return v;
}

// Test fixture: callback.c
//
// (module
//   (func $print (import "" "print") (param i32) (result i32))
//   (func $closure (import "" "closure") (result i32))
//   (func (export "run") (param $x i32) (param $y i32) (result i32)
//     (i32.add
//       (call $print (i32.add (local.get $x) (local.get $y)))
//       (call $closure))))
const std::array<uint8_t, 78> CallbackWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x03, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x02, 0x7f, 0x7f,
    0x01, 0x7f, 0x02, 0x15, 0x02, 0x00, 0x05, 0x70, 0x72, 0x69, 0x6e, 0x74,
    0x00, 0x00, 0x00, 0x07, 0x63, 0x6c, 0x6f, 0x73, 0x75, 0x72, 0x65, 0x00,
    0x01, 0x03, 0x02, 0x01, 0x02, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e,
    0x00, 0x02, 0x0a, 0x0e, 0x01, 0x0c, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a,
    0x10, 0x00, 0x10, 0x01, 0x6a, 0x0b};

int32_t PrintLastArg = 0;
wasm_trap_t *printCallback(const wasm_val_vec_t *args,
                           wasm_val_vec_t *results) {
  PrintLastArg = args->data[0].of.i32;
  wasm_val_copy(&results->data[0], &args->data[0]);
  return nullptr;
}
wasm_trap_t *closureCallback(void *env, const wasm_val_vec_t *,
                             wasm_val_vec_t *results) {
  results->data[0].kind = WASM_I32;
  results->data[0].of.i32 = *static_cast<int32_t *>(env);
  return nullptr;
}

TEST(APIWasmCExampleTest, Callback) {
  PrintLastArg = 0;
  int32_t closure_env = 42;

  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, CallbackWasm.size());
  std::copy(CallbackWasm.begin(), CallbackWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *print_type =
      wasm_functype_new_1_1(wasm_valtype_new_i32(), wasm_valtype_new_i32());
  wasm_func_t *print_func = wasm_func_new(store, print_type, printCallback);
  wasm_functype_delete(print_type);

  wasm_functype_t *closure_type = wasm_functype_new_0_1(wasm_valtype_new_i32());
  wasm_func_t *closure_func = wasm_func_new_with_env(
      store, closure_type, closureCallback, &closure_env, nullptr);
  wasm_functype_delete(closure_type);

  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(print_func),
                                  wasm_func_as_extern(closure_func)};
  wasm_extern_vec_t imports_vec = {2, ext_imports};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  wasm_func_t *run = wasm_extern_as_func(exports.data[0]);

  // run(3, 4) = print(7) + closure() = 7 + 42 = 49.
  wasm_val_t arg_buf[2] = {i32Val(3), i32Val(4)};
  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t results = {1, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(run, &args, &results));
  EXPECT_EQ(49, res_buf[0].of.i32);
  EXPECT_EQ(7, PrintLastArg);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(closure_func);
  wasm_func_delete(print_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

int32_t PrintCppLastArg = 0;
wasm::own<wasm::Trap> printCppCallback(const wasm::vec<wasm::Val> &args,
                                       wasm::vec<wasm::Val> &results) {
  PrintCppLastArg = args[0].i32();
  results[0] = args[0].copy();
  return nullptr;
}
wasm::own<wasm::Trap> closureCppCallback(void *env,
                                         const wasm::vec<wasm::Val> &,
                                         wasm::vec<wasm::Val> &results) {
  results[0] = wasm::Val::i32(*static_cast<int32_t *>(env));
  return nullptr;
}

TEST(APIWasmCExampleTest, CallbackCpp) {
  PrintCppLastArg = 0;
  int32_t closure_env = 42;

  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(CallbackWasm.size());
  std::copy(CallbackWasm.begin(), CallbackWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto print_params = wasm::ownvec<wasm::ValType>::make(
      wasm::ValType::make(wasm::ValKind::I32));
  auto print_results = wasm::ownvec<wasm::ValType>::make(
      wasm::ValType::make(wasm::ValKind::I32));
  auto print_type =
      wasm::FuncType::make(std::move(print_params), std::move(print_results));
  auto print_func =
      wasm::Func::make(store.get(), print_type.get(), printCppCallback);

  auto closure_results = wasm::ownvec<wasm::ValType>::make(
      wasm::ValType::make(wasm::ValKind::I32));
  auto closure_type = wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(),
                                           std::move(closure_results));
  auto closure_func = wasm::Func::make(store.get(), closure_type.get(),
                                       closureCppCallback, &closure_env);

  auto imports =
      wasm::vec<wasm::Extern *>::make(print_func.get(), closure_func.get());
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);

  auto exports = instance->exports();
  auto *run = exports[0]->func();

  // run(3, 4) = print(7) + closure() = 7 + 42 = 49.
  auto args = wasm::vec<wasm::Val>::make(wasm::Val::i32(3), wasm::Val::i32(4));
  auto results = wasm::vec<wasm::Val>::make(wasm::Val(), wasm::Val());
  // Need exactly 1 result slot.
  auto results1 = wasm::vec<wasm::Val>::make(wasm::Val());
  ASSERT_EQ(nullptr, run->call(args, results1));
  EXPECT_EQ(49, results1[0].i32());
  EXPECT_EQ(7, PrintCppLastArg);
}

// Test fixture: finalize.c
//
// (module (func (export "f")) (func (export "g")) (func (export "h")))
const std::array<uint8_t, 47> FinalizeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x04, 0x03, 0x00, 0x00, 0x00, 0x07, 0x0d, 0x03, 0x01,
    0x66, 0x00, 0x00, 0x01, 0x67, 0x00, 0x01, 0x01, 0x68, 0x00, 0x02, 0x0a,
    0x0a, 0x03, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b, 0x02, 0x00, 0x0b};

int FinalizeLiveCount = 0;
void finalizeDec(void *data) {
  (void)data;
  FinalizeLiveCount -= 1;
}

TEST(APIWasmCExampleTest, Finalize) {
  // Spec example runs 100000 iterations; trimmed for a unit test.
  constexpr int Iterations = 1000;
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, FinalizeWasm.size());
  std::copy(FinalizeWasm.begin(), FinalizeWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  FinalizeLiveCount = 0;
  wasm_extern_vec_t no_imports = {0, nullptr};
  for (int i = 0; i < Iterations; ++i) {
    wasm_instance_t *instance =
        wasm_instance_new(store, module, &no_imports, nullptr);
    ASSERT_NE(nullptr, instance);
    wasm_instance_set_host_info_with_finalizer(
        instance, reinterpret_cast<void *>(static_cast<intptr_t>(i)),
        &finalizeDec);
    FinalizeLiveCount += 1;
    wasm_instance_delete(instance);
  }
  EXPECT_EQ(0, FinalizeLiveCount);

  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

int FinalizeCppLiveCount = 0;
void finalizeCppDec(void *data) {
  (void)data;
  FinalizeCppLiveCount -= 1;
}

TEST(APIWasmCExampleTest, FinalizeCpp) {
  // Spec example runs 100000 iterations; trimmed for a unit test.
  constexpr int Iterations = 1000;
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(FinalizeWasm.size());
  std::copy(FinalizeWasm.begin(), FinalizeWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  FinalizeCppLiveCount = 0;
  auto no_imports = wasm::vec<wasm::Extern *>::make();
  for (int i = 0; i < Iterations; ++i) {
    auto instance = wasm::Instance::make(store.get(), module.get(), no_imports);
    ASSERT_NE(nullptr, instance);
    instance->set_host_info(reinterpret_cast<void *>(static_cast<intptr_t>(i)),
                            &finalizeCppDec);
    FinalizeCppLiveCount += 1;
  }
  EXPECT_EQ(0, FinalizeCppLiveCount);
}

// Test fixture: global.c
//
// (module
//   (global $f32_import (import "" "const f32") f32)
//   (global $i64_import (import "" "const i64") i64)
//   (global $mut_f32_import (import "" "var f32") (mut f32))
//   (global $mut_i64_import (import "" "var i64") (mut i64))
//   (global (export "const f32") f32 (f32.const 5))
//   (global (export "const i64") i64 (i64.const 6))
//   (global (export "var f32") (mut f32) (f32.const 7))
//   (global (export "var i64") (mut i64) (i64.const 8))
//   ...12 getter/setter funcs.)
const std::array<uint8_t, 505> GlobalWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x11, 0x04, 0x60,
    0x00, 0x01, 0x7d, 0x60, 0x00, 0x01, 0x7e, 0x60, 0x01, 0x7d, 0x00, 0x60,
    0x01, 0x7e, 0x00, 0x02, 0x35, 0x04, 0x00, 0x09, 0x63, 0x6f, 0x6e, 0x73,
    0x74, 0x20, 0x66, 0x33, 0x32, 0x03, 0x7d, 0x00, 0x00, 0x09, 0x63, 0x6f,
    0x6e, 0x73, 0x74, 0x20, 0x69, 0x36, 0x34, 0x03, 0x7e, 0x00, 0x00, 0x07,
    0x76, 0x61, 0x72, 0x20, 0x66, 0x33, 0x32, 0x03, 0x7d, 0x01, 0x00, 0x07,
    0x76, 0x61, 0x72, 0x20, 0x69, 0x36, 0x34, 0x03, 0x7e, 0x01, 0x03, 0x0d,
    0x0c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02,
    0x03, 0x06, 0x1b, 0x04, 0x7d, 0x00, 0x43, 0x00, 0x00, 0xa0, 0x40, 0x0b,
    0x7e, 0x00, 0x42, 0x06, 0x0b, 0x7d, 0x01, 0x43, 0x00, 0x00, 0xe0, 0x40,
    0x0b, 0x7e, 0x01, 0x42, 0x08, 0x0b, 0x07, 0xb1, 0x02, 0x10, 0x09, 0x63,
    0x6f, 0x6e, 0x73, 0x74, 0x20, 0x66, 0x33, 0x32, 0x03, 0x04, 0x09, 0x63,
    0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 0x36, 0x34, 0x03, 0x05, 0x07, 0x76,
    0x61, 0x72, 0x20, 0x66, 0x33, 0x32, 0x03, 0x06, 0x07, 0x76, 0x61, 0x72,
    0x20, 0x69, 0x36, 0x34, 0x03, 0x07, 0x14, 0x67, 0x65, 0x74, 0x20, 0x63,
    0x6f, 0x6e, 0x73, 0x74, 0x20, 0x66, 0x33, 0x32, 0x20, 0x69, 0x6d, 0x70,
    0x6f, 0x72, 0x74, 0x00, 0x00, 0x14, 0x67, 0x65, 0x74, 0x20, 0x63, 0x6f,
    0x6e, 0x73, 0x74, 0x20, 0x69, 0x36, 0x34, 0x20, 0x69, 0x6d, 0x70, 0x6f,
    0x72, 0x74, 0x00, 0x01, 0x12, 0x67, 0x65, 0x74, 0x20, 0x76, 0x61, 0x72,
    0x20, 0x66, 0x33, 0x32, 0x20, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x00,
    0x02, 0x12, 0x67, 0x65, 0x74, 0x20, 0x76, 0x61, 0x72, 0x20, 0x69, 0x36,
    0x34, 0x20, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x03, 0x14, 0x67,
    0x65, 0x74, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x66, 0x33, 0x32,
    0x20, 0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x04, 0x14, 0x67, 0x65,
    0x74, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x69, 0x36, 0x34, 0x20,
    0x65, 0x78, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x05, 0x12, 0x67, 0x65, 0x74,
    0x20, 0x76, 0x61, 0x72, 0x20, 0x66, 0x33, 0x32, 0x20, 0x65, 0x78, 0x70,
    0x6f, 0x72, 0x74, 0x00, 0x06, 0x12, 0x67, 0x65, 0x74, 0x20, 0x76, 0x61,
    0x72, 0x20, 0x69, 0x36, 0x34, 0x20, 0x65, 0x78, 0x70, 0x6f, 0x72, 0x74,
    0x00, 0x07, 0x12, 0x73, 0x65, 0x74, 0x20, 0x76, 0x61, 0x72, 0x20, 0x66,
    0x33, 0x32, 0x20, 0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x08, 0x12,
    0x73, 0x65, 0x74, 0x20, 0x76, 0x61, 0x72, 0x20, 0x69, 0x36, 0x34, 0x20,
    0x69, 0x6d, 0x70, 0x6f, 0x72, 0x74, 0x00, 0x09, 0x12, 0x73, 0x65, 0x74,
    0x20, 0x76, 0x61, 0x72, 0x20, 0x66, 0x33, 0x32, 0x20, 0x65, 0x78, 0x70,
    0x6f, 0x72, 0x74, 0x00, 0x0a, 0x12, 0x73, 0x65, 0x74, 0x20, 0x76, 0x61,
    0x72, 0x20, 0x66, 0x36, 0x34, 0x20, 0x65, 0x78, 0x70, 0x6f, 0x72, 0x74,
    0x00, 0x0b, 0x0a, 0x45, 0x0c, 0x04, 0x00, 0x23, 0x00, 0x0b, 0x04, 0x00,
    0x23, 0x01, 0x0b, 0x04, 0x00, 0x23, 0x02, 0x0b, 0x04, 0x00, 0x23, 0x03,
    0x0b, 0x04, 0x00, 0x23, 0x04, 0x0b, 0x04, 0x00, 0x23, 0x05, 0x0b, 0x04,
    0x00, 0x23, 0x06, 0x0b, 0x04, 0x00, 0x23, 0x07, 0x0b, 0x06, 0x00, 0x20,
    0x00, 0x24, 0x02, 0x0b, 0x06, 0x00, 0x20, 0x00, 0x24, 0x03, 0x0b, 0x06,
    0x00, 0x20, 0x00, 0x24, 0x06, 0x0b, 0x06, 0x00, 0x20, 0x00, 0x24, 0x07,
    0x0b};

namespace {
// Local helper: call a nullary getter and return its single result.
wasm_val_t callGetter(wasm_func_t *fn) {
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {0, nullptr};
  wasm_val_vec_t results = {1, &out};
  EXPECT_EQ(nullptr, wasm_func_call(fn, &args, &results));
  return out;
}
// Local helper: call a unary setter taking a single value.
void callSetter(wasm_func_t *fn, wasm_val_t arg) {
  wasm_val_vec_t args = {1, &arg};
  wasm_val_vec_t results = {0, nullptr};
  EXPECT_EQ(nullptr, wasm_func_call(fn, &args, &results));
}
} // namespace

TEST(APIWasmCExampleTest, Global) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, GlobalWasm.size());
  std::copy(GlobalWasm.begin(), GlobalWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  // Create the four imported globals.
  wasm_globaltype_t *const_f32_type =
      wasm_globaltype_new(wasm_valtype_new_f32(), WASM_CONST);
  wasm_globaltype_t *const_i64_type =
      wasm_globaltype_new(wasm_valtype_new_i64(), WASM_CONST);
  wasm_globaltype_t *var_f32_type =
      wasm_globaltype_new(wasm_valtype_new_f32(), WASM_VAR);
  wasm_globaltype_t *var_i64_type =
      wasm_globaltype_new(wasm_valtype_new_i64(), WASM_VAR);

  wasm_val_t v1 = f32Val(1.0f);
  wasm_val_t v2 = i64Val(2);
  wasm_val_t v3 = f32Val(3.0f);
  wasm_val_t v4 = i64Val(4);
  wasm_global_t *const_f32_import = wasm_global_new(store, const_f32_type, &v1);
  wasm_global_t *const_i64_import = wasm_global_new(store, const_i64_type, &v2);
  wasm_global_t *var_f32_import = wasm_global_new(store, var_f32_type, &v3);
  wasm_global_t *var_i64_import = wasm_global_new(store, var_i64_type, &v4);
  wasm_globaltype_delete(const_f32_type);
  wasm_globaltype_delete(const_i64_type);
  wasm_globaltype_delete(var_f32_type);
  wasm_globaltype_delete(var_i64_type);

  wasm_extern_t *ext_imports[] = {wasm_global_as_extern(const_f32_import),
                                  wasm_global_as_extern(const_i64_import),
                                  wasm_global_as_extern(var_f32_import),
                                  wasm_global_as_extern(var_i64_import)};
  wasm_extern_vec_t imports_vec = {4, ext_imports};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(16U, exports.size);

  // Export declaration order: 4 exported globals, 8 getters (4 import + 4
  // export), 4 setters (2 import + 2 export).
  wasm_global_t *const_f32_export = wasm_extern_as_global(exports.data[0]);
  wasm_global_t *const_i64_export = wasm_extern_as_global(exports.data[1]);
  wasm_global_t *var_f32_export = wasm_extern_as_global(exports.data[2]);
  wasm_global_t *var_i64_export = wasm_extern_as_global(exports.data[3]);
  wasm_func_t *get_const_f32_import = wasm_extern_as_func(exports.data[4]);
  wasm_func_t *get_const_i64_import = wasm_extern_as_func(exports.data[5]);
  wasm_func_t *get_var_f32_import = wasm_extern_as_func(exports.data[6]);
  wasm_func_t *get_var_i64_import = wasm_extern_as_func(exports.data[7]);
  wasm_func_t *get_const_f32_export = wasm_extern_as_func(exports.data[8]);
  wasm_func_t *get_const_i64_export = wasm_extern_as_func(exports.data[9]);
  wasm_func_t *get_var_f32_export = wasm_extern_as_func(exports.data[10]);
  wasm_func_t *get_var_i64_export = wasm_extern_as_func(exports.data[11]);
  wasm_func_t *set_var_f32_import = wasm_extern_as_func(exports.data[12]);
  wasm_func_t *set_var_i64_import = wasm_extern_as_func(exports.data[13]);
  wasm_func_t *set_var_f32_export = wasm_extern_as_func(exports.data[14]);
  wasm_func_t *set_var_i64_export = wasm_extern_as_func(exports.data[15]);

  // Cloning a global yields a handle pointing at the same underlying global.
  wasm_global_t *copy = wasm_global_copy(var_f32_import);
  EXPECT_TRUE(wasm_global_same(var_f32_import, copy));
  wasm_global_delete(copy);

  // Initial values via host get.
  wasm_val_t out;
  wasm_global_get(const_f32_import, &out);
  EXPECT_FLOAT_EQ(1.0f, out.of.f32);
  wasm_global_get(const_i64_import, &out);
  EXPECT_EQ(2, out.of.i64);
  wasm_global_get(var_f32_import, &out);
  EXPECT_FLOAT_EQ(3.0f, out.of.f32);
  wasm_global_get(var_i64_import, &out);
  EXPECT_EQ(4, out.of.i64);
  wasm_global_get(const_f32_export, &out);
  EXPECT_FLOAT_EQ(5.0f, out.of.f32);
  wasm_global_get(const_i64_export, &out);
  EXPECT_EQ(6, out.of.i64);
  wasm_global_get(var_f32_export, &out);
  EXPECT_FLOAT_EQ(7.0f, out.of.f32);
  wasm_global_get(var_i64_export, &out);
  EXPECT_EQ(8, out.of.i64);

  // Same initial values seen via the wasm-side getter funcs.
  EXPECT_FLOAT_EQ(1.0f, callGetter(get_const_f32_import).of.f32);
  EXPECT_EQ(2, callGetter(get_const_i64_import).of.i64);
  EXPECT_FLOAT_EQ(3.0f, callGetter(get_var_f32_import).of.f32);
  EXPECT_EQ(4, callGetter(get_var_i64_import).of.i64);
  EXPECT_FLOAT_EQ(5.0f, callGetter(get_const_f32_export).of.f32);
  EXPECT_EQ(6, callGetter(get_const_i64_export).of.i64);
  EXPECT_FLOAT_EQ(7.0f, callGetter(get_var_f32_export).of.f32);
  EXPECT_EQ(8, callGetter(get_var_i64_export).of.i64);

  // Round 1: mutate the four var globals via the host API.
  wasm_val_t s33 = f32Val(33.0f);
  wasm_val_t s34 = i64Val(34);
  wasm_val_t s37 = f32Val(37.0f);
  wasm_val_t s38 = i64Val(38);
  wasm_global_set(var_f32_import, &s33);
  wasm_global_set(var_i64_import, &s34);
  wasm_global_set(var_f32_export, &s37);
  wasm_global_set(var_i64_export, &s38);

  wasm_global_get(var_f32_import, &out);
  EXPECT_FLOAT_EQ(33.0f, out.of.f32);
  wasm_global_get(var_i64_import, &out);
  EXPECT_EQ(34, out.of.i64);
  wasm_global_get(var_f32_export, &out);
  EXPECT_FLOAT_EQ(37.0f, out.of.f32);
  wasm_global_get(var_i64_export, &out);
  EXPECT_EQ(38, out.of.i64);
  EXPECT_FLOAT_EQ(33.0f, callGetter(get_var_f32_import).of.f32);
  EXPECT_EQ(34, callGetter(get_var_i64_import).of.i64);
  EXPECT_FLOAT_EQ(37.0f, callGetter(get_var_f32_export).of.f32);
  EXPECT_EQ(38, callGetter(get_var_i64_export).of.i64);

  // Round 2: mutate via the wasm-side setter funcs and re-check.
  callSetter(set_var_f32_import, f32Val(73.0f));
  callSetter(set_var_i64_import, i64Val(74));
  callSetter(set_var_f32_export, f32Val(77.0f));
  callSetter(set_var_i64_export, i64Val(78));

  wasm_global_get(var_f32_import, &out);
  EXPECT_FLOAT_EQ(73.0f, out.of.f32);
  wasm_global_get(var_i64_import, &out);
  EXPECT_EQ(74, out.of.i64);
  wasm_global_get(var_f32_export, &out);
  EXPECT_FLOAT_EQ(77.0f, out.of.f32);
  wasm_global_get(var_i64_export, &out);
  EXPECT_EQ(78, out.of.i64);
  EXPECT_FLOAT_EQ(73.0f, callGetter(get_var_f32_import).of.f32);
  EXPECT_EQ(74, callGetter(get_var_i64_import).of.i64);
  EXPECT_FLOAT_EQ(77.0f, callGetter(get_var_f32_export).of.f32);
  EXPECT_EQ(78, callGetter(get_var_i64_export).of.i64);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_global_delete(var_i64_import);
  wasm_global_delete(var_f32_import);
  wasm_global_delete(const_i64_import);
  wasm_global_delete(const_f32_import);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

namespace {
// Local helper: call a nullary getter on a wasm::Func.
wasm::Val callCppGetter(const wasm::Func *fn) {
  auto args = wasm::vec<wasm::Val>::make();
  auto results = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, results));
  return results[0].copy();
}
// Local helper: call a unary setter on a wasm::Func.
void callCppSetter(const wasm::Func *fn, wasm::Val arg) {
  auto args = wasm::vec<wasm::Val>::make(std::move(arg));
  auto results = wasm::vec<wasm::Val>::make();
  EXPECT_EQ(nullptr, fn->call(args, results));
}
} // namespace

TEST(APIWasmCExampleTest, GlobalCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(GlobalWasm.size());
  std::copy(GlobalWasm.begin(), GlobalWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto const_f32_type = wasm::GlobalType::make(
      wasm::ValType::make(wasm::ValKind::F32), wasm::Mutability::CONST);
  auto const_i64_type = wasm::GlobalType::make(
      wasm::ValType::make(wasm::ValKind::I64), wasm::Mutability::CONST);
  auto var_f32_type = wasm::GlobalType::make(
      wasm::ValType::make(wasm::ValKind::F32), wasm::Mutability::VAR);
  auto var_i64_type = wasm::GlobalType::make(
      wasm::ValType::make(wasm::ValKind::I64), wasm::Mutability::VAR);
  auto const_f32_import =
      wasm::Global::make(store.get(), const_f32_type.get(), wasm::Val::f32(1));
  auto const_i64_import =
      wasm::Global::make(store.get(), const_i64_type.get(), wasm::Val::i64(2));
  auto var_f32_import =
      wasm::Global::make(store.get(), var_f32_type.get(), wasm::Val::f32(3));
  auto var_i64_import =
      wasm::Global::make(store.get(), var_i64_type.get(), wasm::Val::i64(4));

  auto imports = wasm::vec<wasm::Extern *>::make(
      const_f32_import.get(), const_i64_import.get(), var_f32_import.get(),
      var_i64_import.get());
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto exports = instance->exports();
  ASSERT_EQ(16U, exports.size());

  auto *const_f32_export = exports[0]->global();
  auto *const_i64_export = exports[1]->global();
  auto *var_f32_export = exports[2]->global();
  auto *var_i64_export = exports[3]->global();
  auto *get_const_f32_import = exports[4]->func();
  auto *get_const_i64_import = exports[5]->func();
  auto *get_var_f32_import = exports[6]->func();
  auto *get_var_i64_import = exports[7]->func();
  auto *get_const_f32_export = exports[8]->func();
  auto *get_const_i64_export = exports[9]->func();
  auto *get_var_f32_export = exports[10]->func();
  auto *get_var_i64_export = exports[11]->func();
  auto *set_var_f32_import = exports[12]->func();
  auto *set_var_i64_import = exports[13]->func();
  auto *set_var_f32_export = exports[14]->func();
  auto *set_var_i64_export = exports[15]->func();

  // Cloning a global yields a handle pointing at the same underlying global.
  EXPECT_TRUE(var_f32_import->copy()->same(var_f32_import.get()));

  // Initial values via host get.
  EXPECT_FLOAT_EQ(1.0f, const_f32_import->get().f32());
  EXPECT_EQ(2, const_i64_import->get().i64());
  EXPECT_FLOAT_EQ(3.0f, var_f32_import->get().f32());
  EXPECT_EQ(4, var_i64_import->get().i64());
  EXPECT_FLOAT_EQ(5.0f, const_f32_export->get().f32());
  EXPECT_EQ(6, const_i64_export->get().i64());
  EXPECT_FLOAT_EQ(7.0f, var_f32_export->get().f32());
  EXPECT_EQ(8, var_i64_export->get().i64());

  // Same initial values seen via the wasm-side getter funcs.
  EXPECT_FLOAT_EQ(1.0f, callCppGetter(get_const_f32_import).f32());
  EXPECT_EQ(2, callCppGetter(get_const_i64_import).i64());
  EXPECT_FLOAT_EQ(3.0f, callCppGetter(get_var_f32_import).f32());
  EXPECT_EQ(4, callCppGetter(get_var_i64_import).i64());
  EXPECT_FLOAT_EQ(5.0f, callCppGetter(get_const_f32_export).f32());
  EXPECT_EQ(6, callCppGetter(get_const_i64_export).i64());
  EXPECT_FLOAT_EQ(7.0f, callCppGetter(get_var_f32_export).f32());
  EXPECT_EQ(8, callCppGetter(get_var_i64_export).i64());

  // Round 1: mutate via host API.
  var_f32_import->set(wasm::Val::f32(33.0f));
  var_i64_import->set(wasm::Val::i64(34));
  var_f32_export->set(wasm::Val::f32(37.0f));
  var_i64_export->set(wasm::Val::i64(38));

  EXPECT_FLOAT_EQ(33.0f, var_f32_import->get().f32());
  EXPECT_EQ(34, var_i64_import->get().i64());
  EXPECT_FLOAT_EQ(37.0f, var_f32_export->get().f32());
  EXPECT_EQ(38, var_i64_export->get().i64());
  EXPECT_FLOAT_EQ(33.0f, callCppGetter(get_var_f32_import).f32());
  EXPECT_EQ(34, callCppGetter(get_var_i64_import).i64());
  EXPECT_FLOAT_EQ(37.0f, callCppGetter(get_var_f32_export).f32());
  EXPECT_EQ(38, callCppGetter(get_var_i64_export).i64());

  // Round 2: mutate via wasm-side setter funcs.
  callCppSetter(set_var_f32_import, wasm::Val::f32(73.0f));
  callCppSetter(set_var_i64_import, wasm::Val::i64(74));
  callCppSetter(set_var_f32_export, wasm::Val::f32(77.0f));
  callCppSetter(set_var_i64_export, wasm::Val::i64(78));

  EXPECT_FLOAT_EQ(73.0f, var_f32_import->get().f32());
  EXPECT_EQ(74, var_i64_import->get().i64());
  EXPECT_FLOAT_EQ(77.0f, var_f32_export->get().f32());
  EXPECT_EQ(78, var_i64_export->get().i64());
  EXPECT_FLOAT_EQ(73.0f, callCppGetter(get_var_f32_import).f32());
  EXPECT_EQ(74, callCppGetter(get_var_i64_import).i64());
  EXPECT_FLOAT_EQ(77.0f, callCppGetter(get_var_f32_export).f32());
  EXPECT_EQ(78, callCppGetter(get_var_i64_export).i64());
}

// Test fixture: hello.c
//
// (module
//   (func $hello (import "" "hello"))
//   (func (export "run") (call $hello))
// )
const std::array<uint8_t, 47> HelloWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x0a, 0x01, 0x00, 0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75,
    0x6e, 0x00, 0x01, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b};

int HelloCallCount = 0;
wasm_trap_t *helloCallback(const wasm_val_vec_t *, wasm_val_vec_t *) {
  HelloCallCount += 1;
  return nullptr;
}

TEST(APIWasmCExampleTest, Hello) {
  HelloCallCount = 0;

  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, HelloWasm.size());
  std::copy(HelloWasm.begin(), HelloWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *hello_type = wasm_functype_new_0_0();
  wasm_func_t *hello_func = wasm_func_new(store, hello_type, helloCallback);
  wasm_functype_delete(hello_type);

  wasm_extern_t *imports[] = {wasm_func_as_extern(hello_func)};
  wasm_extern_vec_t imports_vec = {1, imports};

  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(1U, exports.size);
  wasm_func_t *run = wasm_extern_as_func(exports.data[0]);
  ASSERT_NE(nullptr, run);

  wasm_val_vec_t args = {0, nullptr};
  wasm_val_vec_t results = {0, nullptr};
  wasm_trap_t *call_trap = wasm_func_call(run, &args, &results);
  EXPECT_EQ(nullptr, call_trap);
  EXPECT_EQ(1, HelloCallCount);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(hello_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

int HelloCppCallCount = 0;
wasm::own<wasm::Trap> helloCppCallback(const wasm::vec<wasm::Val> &,
                                       wasm::vec<wasm::Val> &) {
  HelloCppCallCount += 1;
  return nullptr;
}

TEST(APIWasmCExampleTest, HelloCpp) {
  HelloCppCallCount = 0;

  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(HelloWasm.size());
  std::copy(HelloWasm.begin(), HelloWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto hello_type = wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(),
                                         wasm::ownvec<wasm::ValType>::make());
  auto hello_func =
      wasm::Func::make(store.get(), hello_type.get(), helloCppCallback);

  auto imports = wasm::vec<wasm::Extern *>::make(hello_func.get());

  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto exports = instance->exports();
  ASSERT_EQ(1U, exports.size());
  auto *run = exports[0]->func();
  ASSERT_NE(nullptr, run);

  auto args = wasm::vec<wasm::Val>::make();
  auto results = wasm::vec<wasm::Val>::make();
  auto call_trap = run->call(args, results);
  EXPECT_EQ(nullptr, call_trap);
  EXPECT_EQ(1, HelloCppCallCount);
}

// Test fixture: memory.c
//
// (module
//   (memory (export "memory") 2 3)
//   (func (export "size") (result i32) (memory.size))
//   (func (export "load") (param i32) (result i32) (i32.load8_s (local.get 0)))
//   (func (export "store") (param i32 i32) (i32.store8 (local.get 0) (local.get
//   1))) (data (i32.const 0x1000) "\01\02\03\04"))
const std::array<uint8_t, 110> MemoryWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x03,
    0x60, 0x00, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x60, 0x02,
    0x7f, 0x7f, 0x00, 0x03, 0x04, 0x03, 0x00, 0x01, 0x02, 0x05, 0x04,
    0x01, 0x01, 0x02, 0x03, 0x07, 0x20, 0x04, 0x06, 0x6d, 0x65, 0x6d,
    0x6f, 0x72, 0x79, 0x02, 0x00, 0x04, 0x73, 0x69, 0x7a, 0x65, 0x00,
    0x00, 0x04, 0x6c, 0x6f, 0x61, 0x64, 0x00, 0x01, 0x05, 0x73, 0x74,
    0x6f, 0x72, 0x65, 0x00, 0x02, 0x0a, 0x18, 0x03, 0x04, 0x00, 0x3f,
    0x00, 0x0b, 0x07, 0x00, 0x20, 0x00, 0x2c, 0x00, 0x00, 0x0b, 0x09,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x3a, 0x00, 0x00, 0x0b, 0x0b, 0x0b,
    0x01, 0x00, 0x41, 0x80, 0x20, 0x0b, 0x04, 0x01, 0x02, 0x03, 0x04};

namespace {
// Local helpers mirroring upstream memory.c's check_call/check_trap.
int32_t callLoad(wasm_func_t *fn, int32_t addr) {
  wasm_val_t arg = i32Val(addr);
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {1, &arg};
  wasm_val_vec_t res = {1, &out};
  EXPECT_EQ(nullptr, wasm_func_call(fn, &args, &res));
  return out.of.i32;
}
bool trapsLoad(wasm_func_t *fn, int32_t addr) {
  wasm_val_t arg = i32Val(addr);
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {1, &arg};
  wasm_val_vec_t res = {1, &out};
  wasm_trap_t *trap = wasm_func_call(fn, &args, &res);
  if (trap) {
    wasm_trap_delete(trap);
    return true;
  }
  return false;
}
void okStore(wasm_func_t *fn, int32_t addr, int32_t value) {
  wasm_val_t arg_buf[2] = {i32Val(addr), i32Val(value)};
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t res = {0, nullptr};
  EXPECT_EQ(nullptr, wasm_func_call(fn, &args, &res));
}
bool trapsStore(wasm_func_t *fn, int32_t addr, int32_t value) {
  wasm_val_t arg_buf[2] = {i32Val(addr), i32Val(value)};
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t res = {0, nullptr};
  wasm_trap_t *trap = wasm_func_call(fn, &args, &res);
  if (trap) {
    wasm_trap_delete(trap);
    return true;
  }
  return false;
}
} // namespace

TEST(APIWasmCExampleTest, Memory) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, MemoryWasm.size());
  std::copy(MemoryWasm.begin(), MemoryWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_extern_vec_t imports = {0, nullptr};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance = wasm_instance_new(store, module, &imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(4U, exports.size);
  wasm_memory_t *memory = wasm_extern_as_memory(exports.data[0]);
  wasm_func_t *size_fn = wasm_extern_as_func(exports.data[1]);
  wasm_func_t *load_fn = wasm_extern_as_func(exports.data[2]);
  wasm_func_t *store_fn = wasm_extern_as_func(exports.data[3]);
  ASSERT_NE(nullptr, memory);

  // Cloning a memory yields a handle pointing at the same underlying memory.
  wasm_memory_t *copy = wasm_memory_copy(memory);
  EXPECT_TRUE(wasm_memory_same(memory, copy));
  wasm_memory_delete(copy);

  // Initial size: 2 pages, 0x20000 bytes.
  EXPECT_EQ(2U, wasm_memory_size(memory));
  EXPECT_EQ(0x20000U, wasm_memory_data_size(memory));
  byte_t *data = wasm_memory_data(memory);
  ASSERT_NE(nullptr, data);
  EXPECT_EQ(0, data[0]);
  EXPECT_EQ(0x01, data[0x1000]);
  EXPECT_EQ(0x04, data[0x1003]);

  // size() export reports 2. load probes covering: zero-page, data segment,
  // last in-bounds byte, and first out-of-bounds byte (traps).
  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t no_args = {0, nullptr};
  wasm_val_vec_t res_vec = {1, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(size_fn, &no_args, &res_vec));
  EXPECT_EQ(2, res_buf[0].of.i32);

  EXPECT_EQ(0, callLoad(load_fn, 0));
  EXPECT_EQ(1, callLoad(load_fn, 0x1000));
  EXPECT_EQ(4, callLoad(load_fn, 0x1003));
  EXPECT_EQ(0, callLoad(load_fn, 0x1ffff));
  EXPECT_TRUE(trapsLoad(load_fn, 0x20000));

  // Mutate memory directly then via store; store traps when OOB.
  data[0x1003] = 5;
  okStore(store_fn, 0x1002, 6);
  EXPECT_TRUE(trapsStore(store_fn, 0x20000, 0));
  EXPECT_EQ(6, data[0x1002]);
  EXPECT_EQ(5, data[0x1003]);
  EXPECT_EQ(6, callLoad(load_fn, 0x1002));
  EXPECT_EQ(5, callLoad(load_fn, 0x1003));

  // Grow by 1 page. New page is zeroed and addressable; the next page traps.
  EXPECT_TRUE(wasm_memory_grow(memory, 1));
  EXPECT_EQ(3U, wasm_memory_size(memory));
  EXPECT_EQ(0x30000U, wasm_memory_data_size(memory));
  EXPECT_EQ(0, callLoad(load_fn, 0x20000));
  okStore(store_fn, 0x20000, 0);
  EXPECT_TRUE(trapsLoad(load_fn, 0x30000));
  EXPECT_TRUE(trapsStore(store_fn, 0x30000, 0));

  // grow(1) fails (would exceed declared max of 3); grow(0) is a no-op.
  EXPECT_FALSE(wasm_memory_grow(memory, 1));
  EXPECT_TRUE(wasm_memory_grow(memory, 0));

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_module_delete(module);

  // Stand-alone memory: 5-page min/max, can shrink-grow but not extend.
  wasm_limits_t limits = {5, 5};
  wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
  wasm_memory_t *memory2 = wasm_memory_new(store, memorytype);
  EXPECT_EQ(5U, wasm_memory_size(memory2));
  EXPECT_FALSE(wasm_memory_grow(memory2, 1));
  EXPECT_TRUE(wasm_memory_grow(memory2, 0));
  wasm_memorytype_delete(memorytype);
  wasm_memory_delete(memory2);

  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

namespace {
int32_t callLoadCpp(const wasm::Func *fn, int32_t addr) {
  auto args = wasm::vec<wasm::Val>::make(wasm::Val::i32(addr));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, res));
  return res[0].i32();
}
bool trapsLoadCpp(const wasm::Func *fn, int32_t addr) {
  auto args = wasm::vec<wasm::Val>::make(wasm::Val::i32(addr));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  return fn->call(args, res) != nullptr;
}
void okStoreCpp(const wasm::Func *fn, int32_t addr, int32_t value) {
  auto args =
      wasm::vec<wasm::Val>::make(wasm::Val::i32(addr), wasm::Val::i32(value));
  auto res = wasm::vec<wasm::Val>::make();
  EXPECT_EQ(nullptr, fn->call(args, res));
}
bool trapsStoreCpp(const wasm::Func *fn, int32_t addr, int32_t value) {
  auto args =
      wasm::vec<wasm::Val>::make(wasm::Val::i32(addr), wasm::Val::i32(value));
  auto res = wasm::vec<wasm::Val>::make();
  return fn->call(args, res) != nullptr;
}
} // namespace

TEST(APIWasmCExampleTest, MemoryCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(MemoryWasm.size());
  std::copy(MemoryWasm.begin(), MemoryWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto imports = wasm::vec<wasm::Extern *>::make();
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto exports = instance->exports();
  ASSERT_EQ(4U, exports.size());
  auto *memory = exports[0]->memory();
  auto *size_fn = exports[1]->func();
  auto *load_fn = exports[2]->func();
  auto *store_fn = exports[3]->func();
  ASSERT_NE(nullptr, memory);

  EXPECT_TRUE(memory->copy()->same(memory));

  EXPECT_EQ(2U, memory->size());
  EXPECT_EQ(0x20000U, memory->data_size());
  byte_t *data = memory->data();
  ASSERT_NE(nullptr, data);
  EXPECT_EQ(0, data[0]);
  EXPECT_EQ(0x01, data[0x1000]);
  EXPECT_EQ(0x04, data[0x1003]);

  auto no_args = wasm::vec<wasm::Val>::make();
  auto res_vec = wasm::vec<wasm::Val>::make_uninitialized(1);
  ASSERT_EQ(nullptr, size_fn->call(no_args, res_vec));
  EXPECT_EQ(2, res_vec[0].i32());

  EXPECT_EQ(0, callLoadCpp(load_fn, 0));
  EXPECT_EQ(1, callLoadCpp(load_fn, 0x1000));
  EXPECT_EQ(4, callLoadCpp(load_fn, 0x1003));
  EXPECT_EQ(0, callLoadCpp(load_fn, 0x1ffff));
  EXPECT_TRUE(trapsLoadCpp(load_fn, 0x20000));

  data[0x1003] = 5;
  okStoreCpp(store_fn, 0x1002, 6);
  EXPECT_TRUE(trapsStoreCpp(store_fn, 0x20000, 0));
  EXPECT_EQ(6, data[0x1002]);
  EXPECT_EQ(5, data[0x1003]);
  EXPECT_EQ(6, callLoadCpp(load_fn, 0x1002));
  EXPECT_EQ(5, callLoadCpp(load_fn, 0x1003));

  EXPECT_TRUE(memory->grow(1));
  EXPECT_EQ(3U, memory->size());
  EXPECT_EQ(0x30000U, memory->data_size());
  EXPECT_EQ(0, callLoadCpp(load_fn, 0x20000));
  okStoreCpp(store_fn, 0x20000, 0);
  EXPECT_TRUE(trapsLoadCpp(load_fn, 0x30000));
  EXPECT_TRUE(trapsStoreCpp(store_fn, 0x30000, 0));

  EXPECT_FALSE(memory->grow(1));
  EXPECT_TRUE(memory->grow(0));

  auto memorytype = wasm::MemoryType::make(wasm::Limits(5, 5));
  auto memory2 = wasm::Memory::make(store.get(), memorytype.get());
  EXPECT_EQ(5U, memory2->size());
  EXPECT_FALSE(memory2->grow(1));
  EXPECT_TRUE(memory2->grow(0));
}

// Test fixture: multi.c
//
// (module
//   (func $f (import "" "f") (param i32 i64 i64 i32) (result i32 i64 i64 i32))
//   (func $g (export "g") (param i32 i64 i64 i32) (result i32 i64 i64 i32)
//     (call $f (local.get 0) (local.get 2) (local.get 1) (local.get 3))))
const std::array<uint8_t, 57> MultiWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x01, 0x60,
    0x04, 0x7f, 0x7e, 0x7e, 0x7f, 0x04, 0x7f, 0x7e, 0x7e, 0x7f, 0x02, 0x06,
    0x01, 0x00, 0x01, 0x66, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05,
    0x01, 0x01, 0x67, 0x00, 0x01, 0x0a, 0x0e, 0x01, 0x0c, 0x00, 0x20, 0x00,
    0x20, 0x02, 0x20, 0x01, 0x20, 0x03, 0x10, 0x00, 0x0b};

wasm_trap_t *multiCallback(const wasm_val_vec_t *args,
                           wasm_val_vec_t *results) {
  wasm_val_copy(&results->data[0], &args->data[3]);
  wasm_val_copy(&results->data[1], &args->data[1]);
  wasm_val_copy(&results->data[2], &args->data[2]);
  wasm_val_copy(&results->data[3], &args->data[0]);
  return nullptr;
}

TEST(APIWasmCExampleTest, Multi) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, MultiWasm.size());
  std::copy(MultiWasm.begin(), MultiWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_valtype_t *types[4] = {wasm_valtype_new_i32(), wasm_valtype_new_i64(),
                              wasm_valtype_new_i64(), wasm_valtype_new_i32()};
  wasm_valtype_vec_t tuple1, tuple2;
  wasm_valtype_vec_new(&tuple1, 4, types);
  wasm_valtype_vec_copy(&tuple2, &tuple1);
  wasm_functype_t *callback_type = wasm_functype_new(&tuple1, &tuple2);
  wasm_func_t *callback_func =
      wasm_func_new(store, callback_type, multiCallback);
  wasm_functype_delete(callback_type);

  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(callback_func)};
  wasm_extern_vec_t imports_vec = {1, ext_imports};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(1U, exports.size);
  wasm_func_t *g = wasm_extern_as_func(exports.data[0]);
  ASSERT_NE(nullptr, g);

  // g(1, 2, 3, 4) calls f(1, 3, 2, 4); f permutes to (4, 3, 2, 1).
  wasm_val_t arg_buf[4] = {i32Val(1), i64Val(2), i64Val(3), i32Val(4)};
  wasm_val_t res_buf[4] = {initVal(), initVal(), initVal(), initVal()};
  wasm_val_vec_t args = {4, arg_buf};
  wasm_val_vec_t results = {4, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(g, &args, &results));
  EXPECT_EQ(WASM_I32, results.data[0].kind);
  EXPECT_EQ(4, results.data[0].of.i32);
  EXPECT_EQ(WASM_I64, results.data[1].kind);
  EXPECT_EQ(3, results.data[1].of.i64);
  EXPECT_EQ(WASM_I64, results.data[2].kind);
  EXPECT_EQ(2, results.data[2].of.i64);
  EXPECT_EQ(WASM_I32, results.data[3].kind);
  EXPECT_EQ(1, results.data[3].of.i32);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(callback_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

wasm::own<wasm::Trap> multiCppCallback(const wasm::vec<wasm::Val> &args,
                                       wasm::vec<wasm::Val> &results) {
  results[0] = args[3].copy();
  results[1] = args[1].copy();
  results[2] = args[2].copy();
  results[3] = args[0].copy();
  return nullptr;
}

TEST(APIWasmCExampleTest, MultiCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(MultiWasm.size());
  std::copy(MultiWasm.begin(), MultiWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto tuple = wasm::ownvec<wasm::ValType>::make(
      wasm::ValType::make(wasm::ValKind::I32),
      wasm::ValType::make(wasm::ValKind::I64),
      wasm::ValType::make(wasm::ValKind::I64),
      wasm::ValType::make(wasm::ValKind::I32));
  auto callback_type =
      wasm::FuncType::make(tuple.deep_copy(), tuple.deep_copy());
  auto callback_func =
      wasm::Func::make(store.get(), callback_type.get(), multiCppCallback);

  auto imports = wasm::vec<wasm::Extern *>::make(callback_func.get());
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto exports = instance->exports();
  ASSERT_EQ(1U, exports.size());
  auto *g = exports[0]->func();
  ASSERT_NE(nullptr, g);

  // g(1, 2, 3, 4) calls f(1, 3, 2, 4); f permutes to (4, 3, 2, 1).
  auto args = wasm::vec<wasm::Val>::make(wasm::Val::i32(1), wasm::Val::i64(2),
                                         wasm::Val::i64(3), wasm::Val::i32(4));
  auto results = wasm::vec<wasm::Val>::make_uninitialized(4);
  ASSERT_EQ(nullptr, g->call(args, results));
  EXPECT_EQ(wasm::ValKind::I32, results[0].kind());
  EXPECT_EQ(4, results[0].i32());
  EXPECT_EQ(wasm::ValKind::I64, results[1].kind());
  EXPECT_EQ(3, results[1].i64());
  EXPECT_EQ(wasm::ValKind::I64, results[2].kind());
  EXPECT_EQ(2, results[2].i64());
  EXPECT_EQ(wasm::ValKind::I32, results[3].kind());
  EXPECT_EQ(1, results[3].i32());
}

// Test fixture: reflect.c
//
// (module
//   (func (export "func") (param i32 f64 f32) (result i32) (unreachable))
//   (global (export "global") f64 (f64.const 0))
//   (table (export "table") 0 50 funcref)
//   (memory (export "memory") 1))
const std::array<uint8_t, 92> ReflectWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x01, 0x60,
    0x03, 0x7f, 0x7c, 0x7d, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x04, 0x05,
    0x01, 0x70, 0x01, 0x00, 0x32, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x0d,
    0x01, 0x7c, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0b, 0x07, 0x22, 0x04, 0x04, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x00, 0x06,
    0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x03, 0x00, 0x05, 0x74, 0x61, 0x62,
    0x6c, 0x65, 0x01, 0x00, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02,
    0x00, 0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b};

TEST(APIWasmCExampleTest, Reflect) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, ReflectWasm.size());
  std::copy(ReflectWasm.begin(), ReflectWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_extern_vec_t imports_vec = {0, nullptr};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_exporttype_vec_t export_types;
  wasm_extern_vec_t exports;
  wasm_module_exports(module, &export_types);
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(exports.size, export_types.size);
  ASSERT_EQ(4U, exports.size);

  // For each export, kinds reported by the module-level export type and
  // the runtime extern must agree, and the runtime extern's reified type
  // must match the module-declared kind.
  for (size_t i = 0; i < exports.size; ++i) {
    wasm_externkind_t kind = wasm_extern_kind(exports.data[i]);
    EXPECT_EQ(kind,
              wasm_externtype_kind(wasm_exporttype_type(export_types.data[i])));
    wasm_externtype_t *current = wasm_extern_type(exports.data[i]);
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(kind, wasm_externtype_kind(current));
    wasm_externtype_delete(current);
  }

  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_extern_kind(exports.data[0]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_extern_kind(exports.data[1]));
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_extern_kind(exports.data[2]));
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_extern_kind(exports.data[3]));

  // The exported func has (i32 f64 f32) → (i32) arity.
  wasm_func_t *func = wasm_extern_as_func(exports.data[0]);
  ASSERT_NE(nullptr, func);
  EXPECT_EQ(3U, wasm_func_param_arity(func));
  EXPECT_EQ(1U, wasm_func_result_arity(func));

  wasm_extern_vec_delete(&exports);
  wasm_exporttype_vec_delete(&export_types);
  wasm_instance_delete(instance);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

TEST(APIWasmCExampleTest, ReflectCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(ReflectWasm.size());
  std::copy(ReflectWasm.begin(), ReflectWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto imports = wasm::vec<wasm::Extern *>::make();
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto export_types = module->exports();
  auto exports = instance->exports();
  ASSERT_EQ(exports.size(), export_types.size());
  ASSERT_EQ(4U, exports.size());

  for (size_t i = 0; i < exports.size(); ++i) {
    auto kind = exports[i]->kind();
    EXPECT_EQ(kind, export_types[i]->type()->kind());
    EXPECT_EQ(kind, exports[i]->type()->kind());
  }

  EXPECT_EQ(wasm::ExternKind::FUNC, exports[0]->kind());
  EXPECT_EQ(wasm::ExternKind::GLOBAL, exports[1]->kind());
  EXPECT_EQ(wasm::ExternKind::TABLE, exports[2]->kind());
  EXPECT_EQ(wasm::ExternKind::MEMORY, exports[3]->kind());

  auto *func = exports[0]->func();
  ASSERT_NE(nullptr, func);
  EXPECT_EQ(3U, func->param_arity());
  EXPECT_EQ(1U, func->result_arity());
}

// Test fixture: serialize.c
//
// (module
//   (func $hello (import "" "hello"))
//   (func (export "run") (call $hello)))
const std::array<uint8_t, 47> SerializeWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x02, 0x0a, 0x01, 0x00, 0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75,
    0x6e, 0x00, 0x01, 0x0a, 0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b};

int SerializeHelloCount = 0;
wasm_trap_t *serializeHello(const wasm_val_vec_t *, wasm_val_vec_t *) {
  SerializeHelloCount += 1;
  return nullptr;
}

TEST(APIWasmCExampleTest, Serialize) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, SerializeWasm.size());
  std::copy(SerializeWasm.begin(), SerializeWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_byte_vec_t serialized;
  wasm_module_serialize(module, &serialized);
  ASSERT_NE(nullptr, serialized.data);
  ASSERT_LT(0U, serialized.size);
  // Serializer output is canonical wasm binary; verify the magic header.
  ASSERT_GE(serialized.size, 4U);
  EXPECT_EQ('\0', serialized.data[0]);
  EXPECT_EQ('a', serialized.data[1]);
  EXPECT_EQ('s', serialized.data[2]);
  EXPECT_EQ('m', serialized.data[3]);
  wasm_module_delete(module);

  wasm_module_t *deserialized = wasm_module_deserialize(store, &serialized);
  wasm_byte_vec_delete(&serialized);
  ASSERT_NE(nullptr, deserialized);

  wasm_functype_t *hello_type = wasm_functype_new_0_0();
  wasm_func_t *hello_func = wasm_func_new(store, hello_type, serializeHello);
  wasm_functype_delete(hello_type);

  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(hello_func)};
  wasm_extern_vec_t imports_vec = {1, ext_imports};
  wasm_instance_t *instance =
      wasm_instance_new(store, deserialized, &imports_vec, nullptr);
  ASSERT_NE(nullptr, instance);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(1U, exports.size);
  wasm_func_t *run_export = wasm_extern_as_func(exports.data[0]);
  ASSERT_NE(nullptr, run_export);

  SerializeHelloCount = 0;
  wasm_val_vec_t no_args = {0, nullptr};
  wasm_val_vec_t no_results = {0, nullptr};
  ASSERT_EQ(nullptr, wasm_func_call(run_export, &no_args, &no_results));
  EXPECT_EQ(1, SerializeHelloCount);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(hello_func);
  wasm_module_delete(deserialized);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

int SerializeCppHelloCount = 0;
wasm::own<wasm::Trap> serializeCppHello(const wasm::vec<wasm::Val> &,
                                        wasm::vec<wasm::Val> &) {
  SerializeCppHelloCount += 1;
  return nullptr;
}

TEST(APIWasmCExampleTest, SerializeCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(SerializeWasm.size());
  std::copy(SerializeWasm.begin(), SerializeWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto serialized = module->serialize();
  ASSERT_NE(nullptr, serialized.get());
  ASSERT_LT(0U, serialized.size());
  module.reset();

  auto deserialized = wasm::Module::deserialize(store.get(), serialized);
  ASSERT_NE(nullptr, deserialized);

  auto hello_type = wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(),
                                         wasm::ownvec<wasm::ValType>::make());
  auto hello_func =
      wasm::Func::make(store.get(), hello_type.get(), serializeCppHello);

  auto imports = wasm::vec<wasm::Extern *>::make(hello_func.get());
  auto instance =
      wasm::Instance::make(store.get(), deserialized.get(), imports);
  ASSERT_NE(nullptr, instance);

  auto exports = instance->exports();
  ASSERT_EQ(1U, exports.size());
  auto *run_export = exports[0]->func();
  ASSERT_NE(nullptr, run_export);

  SerializeCppHelloCount = 0;
  auto no_args = wasm::vec<wasm::Val>::make();
  auto no_results = wasm::vec<wasm::Val>::make();
  ASSERT_EQ(nullptr, run_export->call(no_args, no_results));
  EXPECT_EQ(1, SerializeCppHelloCount);
}

// Test fixture: start.c
//
// (module (func $start (unreachable)) (start $start))
const std::array<uint8_t, 28> StartWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04,
    0x01, 0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x08, 0x01,
    0x00, 0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b};

TEST(APIWasmCExampleTest, Start) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, StartWasm.size());
  std::copy(StartWasm.begin(), StartWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_extern_vec_t no_imports = {0, nullptr};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &no_imports, &trap);
  EXPECT_EQ(nullptr, instance);
  ASSERT_NE(nullptr, trap);
  wasm_message_t msg;
  wasm_trap_message(trap, &msg);
  EXPECT_NE(std::string(msg.data, msg.size).find("unreachable"),
            std::string::npos);
  // Start function is funcidx 0 (the only function).
  wasm_frame_t *origin = wasm_trap_origin(trap);
  ASSERT_NE(nullptr, origin);
  EXPECT_EQ(0U, wasm_frame_func_index(origin));
  wasm_frame_delete(origin);
  wasm_byte_vec_delete(&msg);
  wasm_trap_delete(trap);

  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

TEST(APIWasmCExampleTest, StartCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(StartWasm.size());
  std::copy(StartWasm.begin(), StartWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto no_imports = wasm::vec<wasm::Extern *>::make();
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), no_imports, &trap);
  EXPECT_EQ(nullptr, instance);
  ASSERT_NE(nullptr, trap);
  auto msg = trap->message();
  EXPECT_NE(std::string(msg.get(), msg.size()).find("unreachable"),
            std::string::npos);
  // Start function is funcidx 0 (the only function).
  auto origin = trap->origin();
  ASSERT_NE(nullptr, origin);
  EXPECT_EQ(0U, origin->func_index());
}

// Test fixture: table.c
//
// (module
//   (table (export "table") 2 10 funcref)
//   (func (export "call_indirect") (param i32 i32) (result i32)
//     (call_indirect (param i32) (result i32) (local.get 0) (local.get 1)))
//   (func $f (export "f") (param i32) (result i32) (local.get 0))
//   (func (export "g") (param i32) (result i32) (i32.const 666))
//   (elem (i32.const 1) $f))
const std::array<uint8_t, 103> TableWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x03, 0x04,
    0x03, 0x00, 0x01, 0x01, 0x04, 0x05, 0x01, 0x70, 0x01, 0x02, 0x0a, 0x07,
    0x21, 0x04, 0x05, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x01, 0x00, 0x0d, 0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x69, 0x6e, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74,
    0x00, 0x00, 0x01, 0x66, 0x00, 0x01, 0x01, 0x67, 0x00, 0x02, 0x09, 0x07,
    0x01, 0x00, 0x41, 0x01, 0x0b, 0x01, 0x01, 0x0a, 0x16, 0x03, 0x09, 0x00,
    0x20, 0x00, 0x20, 0x01, 0x11, 0x01, 0x00, 0x0b, 0x04, 0x00, 0x20, 0x00,
    0x0b, 0x05, 0x00, 0x41, 0x9a, 0x05, 0x0b};

wasm_trap_t *tableNegCallback(const wasm_val_vec_t *args,
                              wasm_val_vec_t *results) {
  results->data[0].kind = WASM_I32;
  results->data[0].of.i32 = -args->data[0].of.i32;
  return nullptr;
}

namespace {
int32_t callIndirect(wasm_func_t *fn, int32_t arg1, int32_t arg2) {
  wasm_val_t arg_buf[2] = {i32Val(arg1), i32Val(arg2)};
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t res = {1, &out};
  EXPECT_EQ(nullptr, wasm_func_call(fn, &args, &res));
  return out.of.i32;
}
bool indirectTraps(wasm_func_t *fn, int32_t arg1, int32_t arg2) {
  wasm_val_t arg_buf[2] = {i32Val(arg1), i32Val(arg2)};
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t res = {1, &out};
  wasm_trap_t *trap = wasm_func_call(fn, &args, &res);
  if (trap) {
    wasm_trap_delete(trap);
    return true;
  }
  return false;
}
} // namespace

TEST(APIWasmCExampleTest, Table) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, TableWasm.size());
  std::copy(TableWasm.begin(), TableWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_extern_vec_t imports = {0, nullptr};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance = wasm_instance_new(store, module, &imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(4U, exports.size);
  wasm_table_t *table = wasm_extern_as_table(exports.data[0]);
  wasm_func_t *call_indirect_fn = wasm_extern_as_func(exports.data[1]);
  wasm_func_t *fn_f = wasm_extern_as_func(exports.data[2]);
  wasm_func_t *fn_g = wasm_extern_as_func(exports.data[3]);
  ASSERT_NE(nullptr, table);
  ASSERT_NE(nullptr, call_indirect_fn);
  ASSERT_NE(nullptr, fn_f);
  ASSERT_NE(nullptr, fn_g);

  // Host-side negate callback; later stored at table[3].
  wasm_functype_t *neg_type =
      wasm_functype_new_1_1(wasm_valtype_new_i32(), wasm_valtype_new_i32());
  wasm_func_t *fn_h = wasm_func_new(store, neg_type, tableNegCallback);
  wasm_functype_delete(neg_type);

  // Cloning the table yields a handle pointing at the same underlying table.
  wasm_table_t *copy = wasm_table_copy(table);
  EXPECT_TRUE(wasm_table_same(table, copy));
  wasm_table_delete(copy);

  // Initial: size 2, slot 0 null (elem starts at offset 1), slot 1 = $f.
  EXPECT_EQ(2U, wasm_table_size(table));
  wasm_ref_t *r0 = wasm_table_get(table, 0);
  EXPECT_EQ(nullptr, r0);
  wasm_ref_t *r1 = wasm_table_get(table, 1);
  EXPECT_NE(nullptr, r1);
  if (r1) {
    wasm_ref_delete(r1);
  }
  // call_indirect(0, 0) traps (null slot); call_indirect(7, 1) returns 7;
  // call_indirect(0, 2) traps (out of bounds).
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 0));
  EXPECT_EQ(7, callIndirect(call_indirect_fn, 7, 1));
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 2));

  // Mutate: set[0]=g, set[1]=null, set[2]=f fails (OOB).
  EXPECT_TRUE(wasm_table_set(table, 0, wasm_func_as_ref(fn_g)));
  EXPECT_TRUE(wasm_table_set(table, 1, nullptr));
  EXPECT_FALSE(wasm_table_set(table, 2, wasm_func_as_ref(fn_f)));
  wasm_ref_t *r0b = wasm_table_get(table, 0);
  EXPECT_NE(nullptr, r0b);
  if (r0b) {
    wasm_ref_delete(r0b);
  }
  wasm_ref_t *r1b = wasm_table_get(table, 1);
  EXPECT_EQ(nullptr, r1b);
  EXPECT_EQ(666, callIndirect(call_indirect_fn, 7, 0));
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 1));
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 2));

  // Grow by 3 to size 5; populate slots 2 and 3; OOB set still fails.
  EXPECT_TRUE(wasm_table_grow(table, 3, nullptr));
  EXPECT_EQ(5U, wasm_table_size(table));
  EXPECT_TRUE(wasm_table_set(table, 2, wasm_func_as_ref(fn_f)));
  EXPECT_TRUE(wasm_table_set(table, 3, wasm_func_as_ref(fn_h)));
  EXPECT_FALSE(wasm_table_set(table, 5, nullptr));
  EXPECT_EQ(5, callIndirect(call_indirect_fn, 5, 2));
  EXPECT_EQ(-6, callIndirect(call_indirect_fn, 6, 3));
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 4));
  EXPECT_TRUE(indirectTraps(call_indirect_fn, 0, 5));

  // Grow with a fill ref; new slots 5 and 6 reference $f.
  EXPECT_TRUE(wasm_table_grow(table, 2, wasm_func_as_ref(fn_f)));
  EXPECT_EQ(7U, wasm_table_size(table));
  wasm_ref_t *r5 = wasm_table_get(table, 5);
  EXPECT_NE(nullptr, r5);
  if (r5) {
    wasm_ref_delete(r5);
  }
  wasm_ref_t *r6 = wasm_table_get(table, 6);
  EXPECT_NE(nullptr, r6);
  if (r6) {
    wasm_ref_delete(r6);
  }

  // grow(5) fails (declared max is 10, current 7; growing 5 → 12 exceeds).
  EXPECT_FALSE(wasm_table_grow(table, 5, nullptr));
  EXPECT_TRUE(wasm_table_grow(table, 3, nullptr));
  EXPECT_TRUE(wasm_table_grow(table, 0, nullptr));

  // OOB get returns null.
  EXPECT_EQ(nullptr, wasm_table_get(table, 100));

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(fn_h);
  wasm_module_delete(module);

  // Stand-alone table: 5-slot min/max, grow(1) fails, grow(0) succeeds.
  wasm_limits_t limits = {5, 5};
  wasm_tabletype_t *tabletype =
      wasm_tabletype_new(wasm_valtype_new_funcref(), &limits);
  wasm_table_t *table2 = wasm_table_new(store, tabletype, nullptr);
  EXPECT_EQ(5U, wasm_table_size(table2));
  EXPECT_FALSE(wasm_table_grow(table2, 1, nullptr));
  EXPECT_TRUE(wasm_table_grow(table2, 0, nullptr));
  wasm_tabletype_delete(tabletype);
  wasm_table_delete(table2);

  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

wasm::own<wasm::Trap> tableCppNegCallback(const wasm::vec<wasm::Val> &args,
                                          wasm::vec<wasm::Val> &results) {
  results[0] = wasm::Val::i32(-args[0].i32());
  return nullptr;
}

namespace {
int32_t callIndirectCpp(const wasm::Func *fn, int32_t arg1, int32_t arg2) {
  auto args =
      wasm::vec<wasm::Val>::make(wasm::Val::i32(arg1), wasm::Val::i32(arg2));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, res));
  return res[0].i32();
}
bool indirectTrapsCpp(const wasm::Func *fn, int32_t arg1, int32_t arg2) {
  auto args =
      wasm::vec<wasm::Val>::make(wasm::Val::i32(arg1), wasm::Val::i32(arg2));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  return fn->call(args, res) != nullptr;
}
} // namespace

TEST(APIWasmCExampleTest, TableCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(TableWasm.size());
  std::copy(TableWasm.begin(), TableWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto imports = wasm::vec<wasm::Extern *>::make();
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  auto exports = instance->exports();
  ASSERT_EQ(4U, exports.size());
  auto *table = exports[0]->table();
  auto *call_indirect_fn = exports[1]->func();
  auto *fn_f = exports[2]->func();
  auto *fn_g = exports[3]->func();
  ASSERT_NE(nullptr, table);

  auto neg_type =
      wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(
                               wasm::ValType::make(wasm::ValKind::I32)),
                           wasm::ownvec<wasm::ValType>::make(
                               wasm::ValType::make(wasm::ValKind::I32)));
  auto fn_h =
      wasm::Func::make(store.get(), neg_type.get(), tableCppNegCallback);

  EXPECT_TRUE(table->copy()->same(table));

  // Initial: size 2, slot 0 null, slot 1 = $f.
  EXPECT_EQ(2U, table->size());
  EXPECT_EQ(nullptr, table->get(0));
  EXPECT_NE(nullptr, table->get(1));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 0));
  EXPECT_EQ(7, callIndirectCpp(call_indirect_fn, 7, 1));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 2));

  EXPECT_TRUE(table->set(0, fn_g));
  EXPECT_TRUE(table->set(1, nullptr));
  EXPECT_FALSE(table->set(2, fn_f));
  EXPECT_NE(nullptr, table->get(0));
  EXPECT_EQ(nullptr, table->get(1));
  EXPECT_EQ(666, callIndirectCpp(call_indirect_fn, 7, 0));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 1));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 2));

  EXPECT_TRUE(table->grow(3));
  EXPECT_EQ(5U, table->size());
  EXPECT_TRUE(table->set(2, fn_f));
  EXPECT_TRUE(table->set(3, fn_h.get()));
  EXPECT_FALSE(table->set(5, nullptr));
  EXPECT_EQ(5, callIndirectCpp(call_indirect_fn, 5, 2));
  EXPECT_EQ(-6, callIndirectCpp(call_indirect_fn, 6, 3));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 4));
  EXPECT_TRUE(indirectTrapsCpp(call_indirect_fn, 0, 5));

  EXPECT_TRUE(table->grow(2, fn_f));
  EXPECT_EQ(7U, table->size());
  EXPECT_NE(nullptr, table->get(5));
  EXPECT_NE(nullptr, table->get(6));

  EXPECT_FALSE(table->grow(5));
  EXPECT_TRUE(table->grow(3));
  EXPECT_TRUE(table->grow(0));

  EXPECT_EQ(nullptr, table->get(100));

  // Stand-alone table.
  auto tabletype = wasm::TableType::make(
      wasm::ValType::make(wasm::ValKind::FUNCREF), wasm::Limits(5, 5));
  auto table2 = wasm::Table::make(store.get(), tabletype.get());
  EXPECT_EQ(5U, table2->size());
  EXPECT_FALSE(table2->grow(1));
  EXPECT_TRUE(table2->grow(0));
}

// Test fixture: trap.c
//
// (module
//   (func $callback (import "" "callback") (result i32))
//   (func (export "callback") (result i32) (call $callback))
//   (func (export "unreachable") (result i32) (unreachable) (i32.const 1)))
const std::array<uint8_t, 77> TrapWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01,
    0x60, 0x00, 0x01, 0x7f, 0x02, 0x0d, 0x01, 0x00, 0x08, 0x63, 0x61,
    0x6c, 0x6c, 0x62, 0x61, 0x63, 0x6b, 0x00, 0x00, 0x03, 0x03, 0x02,
    0x00, 0x00, 0x07, 0x1a, 0x02, 0x08, 0x63, 0x61, 0x6c, 0x6c, 0x62,
    0x61, 0x63, 0x6b, 0x00, 0x01, 0x0b, 0x75, 0x6e, 0x72, 0x65, 0x61,
    0x63, 0x68, 0x61, 0x62, 0x6c, 0x65, 0x00, 0x02, 0x0a, 0x0c, 0x02,
    0x04, 0x00, 0x10, 0x00, 0x0b, 0x05, 0x00, 0x00, 0x41, 0x01, 0x0b};

wasm_trap_t *failCallback(void *env, const wasm_val_vec_t *, wasm_val_vec_t *) {
  wasm_name_t message;
  wasm_name_new_from_string_nt(&message, "callback abort");
  wasm_trap_t *trap = wasm_trap_new(static_cast<wasm_store_t *>(env), &message);
  wasm_name_delete(&message);
  return trap;
}

TEST(APIWasmCExampleTest, Trap) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, TrapWasm.size());
  std::copy(TrapWasm.begin(), TrapWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *cb_type = wasm_functype_new_0_1(wasm_valtype_new_i32());
  wasm_func_t *cb_func =
      wasm_func_new_with_env(store, cb_type, failCallback, store, nullptr);
  wasm_functype_delete(cb_type);

  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(cb_func)};
  wasm_extern_vec_t imports_vec = {1, ext_imports};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(2U, exports.size);
  wasm_func_t *callback_export = wasm_extern_as_func(exports.data[0]);
  wasm_func_t *unreachable_export = wasm_extern_as_func(exports.data[1]);

  // Host trap message is dropped; caller sees synthesized "host function
  // failed". Origin is the called export, funcidx 1.
  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t no_args = {0, nullptr};
  wasm_val_vec_t results = {1, res_buf};
  wasm_trap_t *t1 = wasm_func_call(callback_export, &no_args, &results);
  ASSERT_NE(nullptr, t1);
  wasm_message_t msg1;
  wasm_trap_message(t1, &msg1);
  EXPECT_NE(std::string(msg1.data, msg1.size).find("host function failed"),
            std::string::npos);
  wasm_frame_t *origin1 = wasm_trap_origin(t1);
  ASSERT_NE(nullptr, origin1);
  EXPECT_EQ(1U, wasm_frame_func_index(origin1));
  wasm_frame_delete(origin1);
  wasm_byte_vec_delete(&msg1);
  wasm_trap_delete(t1);

  // Wasm-side trap. Origin = funcidx 2; trace is a single-element span.
  wasm_trap_t *t2 = wasm_func_call(unreachable_export, &no_args, &results);
  ASSERT_NE(nullptr, t2);
  wasm_message_t msg2;
  wasm_trap_message(t2, &msg2);
  EXPECT_NE(std::string(msg2.data, msg2.size).find("unreachable"),
            std::string::npos);
  wasm_frame_t *origin2 = wasm_trap_origin(t2);
  ASSERT_NE(nullptr, origin2);
  EXPECT_EQ(2U, wasm_frame_func_index(origin2));
  wasm_frame_delete(origin2);
  wasm_frame_vec_t trace;
  wasm_trap_trace(t2, &trace);
  EXPECT_EQ(1U, trace.size);
  wasm_frame_vec_delete(&trace);
  wasm_byte_vec_delete(&msg2);
  wasm_trap_delete(t2);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(cb_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

wasm::own<wasm::Trap> failCppCallback(void *env, const wasm::vec<wasm::Val> &,
                                      wasm::vec<wasm::Val> &) {
  auto *store = static_cast<wasm::Store *>(env);
  return wasm::Trap::make(store, wasm::Message::make_nt("callback abort"));
}

TEST(APIWasmCExampleTest, TrapCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(TrapWasm.size());
  std::copy(TrapWasm.begin(), TrapWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto cb_results = wasm::ownvec<wasm::ValType>::make(
      wasm::ValType::make(wasm::ValKind::I32));
  auto cb_type = wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(),
                                      std::move(cb_results));
  auto cb_func = wasm::Func::make(store.get(), cb_type.get(), failCppCallback,
                                  store.get());

  auto imports = wasm::vec<wasm::Extern *>::make(cb_func.get());
  wasm::own<wasm::Trap> trap;
  auto instance =
      wasm::Instance::make(store.get(), module.get(), imports, &trap);
  ASSERT_NE(nullptr, instance);

  auto exports = instance->exports();
  ASSERT_EQ(2U, exports.size());
  auto *callback_export = exports[0]->func();
  auto *unreachable_export = exports[1]->func();

  // Host trap message is dropped; caller sees synthesized "host function
  // failed". Origin is the called export, funcidx 1.
  auto no_args = wasm::vec<wasm::Val>::make();
  auto results = wasm::vec<wasm::Val>::make(wasm::Val());
  auto t1 = callback_export->call(no_args, results);
  ASSERT_NE(nullptr, t1);
  auto msg1 = t1->message();
  EXPECT_NE(std::string(msg1.get(), msg1.size()).find("host function failed"),
            std::string::npos);
  auto origin1 = t1->origin();
  ASSERT_NE(nullptr, origin1);
  EXPECT_EQ(1U, origin1->func_index());

  // Wasm-side trap. Origin = funcidx 2; trace is a single-element span.
  auto t2 = unreachable_export->call(no_args, results);
  ASSERT_NE(nullptr, t2);
  auto msg2 = t2->message();
  EXPECT_NE(std::string(msg2.get(), msg2.size()).find("unreachable"),
            std::string::npos);
  auto origin2 = t2->origin();
  ASSERT_NE(nullptr, origin2);
  EXPECT_EQ(2U, origin2->func_index());
  auto trace = t2->trace();
  EXPECT_EQ(1U, trace.size());
}

// Test fixture: threads.c — share a compiled module across worker threads,
// each obtaining a per-store module handle and running an instance multiple
// times.
//
// (module
//   (func $message (import "" "hello") (param i32))
//   (global $id (import "" "id") i32)
//   (func (export "run") (call $message (global.get $id))))
const std::array<uint8_t, 60> ThreadsWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x11, 0x02, 0x00, 0x05, 0x68,
    0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x00, 0x00, 0x02, 0x69, 0x64, 0x03, 0x7f,
    0x00, 0x03, 0x02, 0x01, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e,
    0x00, 0x01, 0x0a, 0x08, 0x01, 0x06, 0x00, 0x23, 0x00, 0x10, 0x00, 0x0b};

// Spec example runs 10 threads × 3 reps; trimmed for a unit test.
constexpr int ThreadsCount = 4;
constexpr int ThreadsReps = 2;

std::atomic<int> ThreadsHelloCount{0};
std::atomic<int64_t> ThreadsIdSum{0};
wasm_trap_t *threadsHelloCallback(const wasm_val_vec_t *args,
                                  wasm_val_vec_t *) {
  ThreadsIdSum.fetch_add(args->data[0].of.i32, std::memory_order_relaxed);
  ThreadsHelloCount.fetch_add(1, std::memory_order_relaxed);
  return nullptr;
}

TEST(APIWasmCExampleTest, Threads) {
  ThreadsHelloCount.store(0);
  ThreadsIdSum.store(0);

  wasm_engine_t *engine = wasm_engine_new();

  // Compile once, share, then dispose of the temp store/module used to
  // build the shared handle.
  wasm_shared_module_t *shared = nullptr;
  {
    wasm_store_t *tmp_store = wasm_store_new(engine);
    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, ThreadsWasm.size());
    std::copy(ThreadsWasm.begin(), ThreadsWasm.end(), binary.data);
    wasm_module_t *module = wasm_module_new(tmp_store, &binary);
    wasm_byte_vec_delete(&binary);
    ASSERT_NE(nullptr, module);
    shared = wasm_module_share(module);
    wasm_module_delete(module);
    wasm_store_delete(tmp_store);
  }
  ASSERT_NE(nullptr, shared);

  std::vector<std::thread> workers;
  workers.reserve(ThreadsCount);
  for (int id = 0; id < ThreadsCount; ++id) {
    workers.emplace_back([engine, shared, id]() {
      wasm_store_t *store = wasm_store_new(engine);
      wasm_module_t *module = wasm_module_obtain(store, shared);
      ASSERT_NE(nullptr, module);

      for (int rep = 0; rep < ThreadsReps; ++rep) {
        wasm_functype_t *func_type =
            wasm_functype_new_1_0(wasm_valtype_new_i32());
        wasm_func_t *func =
            wasm_func_new(store, func_type, threadsHelloCallback);
        wasm_functype_delete(func_type);

        wasm_val_t val = i32Val(id);
        wasm_globaltype_t *global_type =
            wasm_globaltype_new(wasm_valtype_new_i32(), WASM_CONST);
        wasm_global_t *global = wasm_global_new(store, global_type, &val);
        wasm_globaltype_delete(global_type);

        wasm_extern_t *ext_imports[] = {wasm_func_as_extern(func),
                                        wasm_global_as_extern(global)};
        wasm_extern_vec_t imports_vec = {2, ext_imports};
        wasm_instance_t *instance =
            wasm_instance_new(store, module, &imports_vec, nullptr);
        ASSERT_NE(nullptr, instance);
        // Match upstream threads.c: the instance now retains the imports, so
        // releasing the host handles immediately is safe.
        wasm_func_delete(func);
        wasm_global_delete(global);

        wasm_extern_vec_t exports;
        wasm_instance_exports(instance, &exports);
        ASSERT_EQ(1U, exports.size);
        wasm_func_t *run_func = wasm_extern_as_func(exports.data[0]);
        ASSERT_NE(nullptr, run_func);

        wasm_val_vec_t no_args = {0, nullptr};
        wasm_val_vec_t no_res = {0, nullptr};
        EXPECT_EQ(nullptr, wasm_func_call(run_func, &no_args, &no_res));

        wasm_extern_vec_delete(&exports);
        wasm_instance_delete(instance);
      }

      wasm_module_delete(module);
      wasm_store_delete(store);
    });
  }
  for (auto &t : workers) {
    t.join();
  }

  EXPECT_EQ(ThreadsCount * ThreadsReps, ThreadsHelloCount.load());
  // Each thread invokes run() ThreadsReps times with id; expected sum is
  // ThreadsReps * (0+1+...+ThreadsCount-1).
  int64_t expected_sum = static_cast<int64_t>(ThreadsReps) *
                         (ThreadsCount * (ThreadsCount - 1) / 2);
  EXPECT_EQ(expected_sum, ThreadsIdSum.load());

  wasm_shared_module_delete(shared);
  wasm_engine_delete(engine);
}

std::atomic<int> ThreadsCppHelloCount{0};
std::atomic<int64_t> ThreadsCppIdSum{0};
wasm::own<wasm::Trap> threadsCppHelloCallback(const wasm::vec<wasm::Val> &args,
                                              wasm::vec<wasm::Val> &) {
  ThreadsCppIdSum.fetch_add(args[0].i32(), std::memory_order_relaxed);
  ThreadsCppHelloCount.fetch_add(1, std::memory_order_relaxed);
  return nullptr;
}

TEST(APIWasmCExampleTest, ThreadsCpp) {
  ThreadsCppHelloCount.store(0);
  ThreadsCppIdSum.store(0);

  auto engine = wasm::Engine::make();

  wasm::own<wasm::Shared<wasm::Module>> shared;
  {
    auto tmp_store = wasm::Store::make(engine.get());
    auto binary = wasm::vec<byte_t>::make_uninitialized(ThreadsWasm.size());
    std::copy(ThreadsWasm.begin(), ThreadsWasm.end(), binary.get());
    auto module = wasm::Module::make(tmp_store.get(), binary);
    ASSERT_NE(nullptr, module);
    shared = module->share();
  }
  ASSERT_NE(nullptr, shared);

  std::vector<std::thread> workers;
  workers.reserve(ThreadsCount);
  for (int id = 0; id < ThreadsCount; ++id) {
    workers.emplace_back([&shared, &engine, id]() {
      auto store = wasm::Store::make(engine.get());
      auto module = wasm::Module::obtain(store.get(), shared.get());
      ASSERT_NE(nullptr, module);

      for (int rep = 0; rep < ThreadsReps; ++rep) {
        auto func_type =
            wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(
                                     wasm::ValType::make(wasm::ValKind::I32)),
                                 wasm::ownvec<wasm::ValType>::make());
        auto func = wasm::Func::make(store.get(), func_type.get(),
                                     threadsCppHelloCallback);
        auto global_type = wasm::GlobalType::make(
            wasm::ValType::make(wasm::ValKind::I32), wasm::Mutability::CONST);
        auto global = wasm::Global::make(store.get(), global_type.get(),
                                         wasm::Val::i32(id));

        auto imports =
            wasm::vec<wasm::Extern *>::make(func.get(), global.get());
        auto instance =
            wasm::Instance::make(store.get(), module.get(), imports);
        ASSERT_NE(nullptr, instance);

        auto exports = instance->exports();
        ASSERT_EQ(1U, exports.size());
        auto *run_func = exports[0]->func();
        ASSERT_NE(nullptr, run_func);

        auto no_args = wasm::vec<wasm::Val>::make();
        auto no_res = wasm::vec<wasm::Val>::make();
        EXPECT_EQ(nullptr, run_func->call(no_args, no_res));
      }
    });
  }
  for (auto &t : workers) {
    t.join();
  }

  EXPECT_EQ(ThreadsCount * ThreadsReps, ThreadsCppHelloCount.load());
  int64_t expected_sum = static_cast<int64_t>(ThreadsReps) *
                         (ThreadsCount * (ThreadsCount - 1) / 2);
  EXPECT_EQ(expected_sum, ThreadsCppIdSum.load());
}

// Test fixture: hostref.c — pass host references (externref) through wasm
// globals, tables, and a host callback that just echoes its arg.
//
// (module
//   (import "" "f" (func $fun (param externref) (result externref)))
//   (global $glob (export "global") (mut externref) (ref.null extern))
//   (table $tab (export "table") 10 externref)
//   (func (export "global.set") (param externref) ...)
//   (func (export "global.get") (result externref) ...)
//   (func (export "table.set") (param i32 externref) ...)
//   (func (export "table.get") (param i32) (result externref) ...)
//   (func (export "func.call") (param externref) (result externref) ...))
const std::array<uint8_t, 184> HostrefWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x18, 0x05, 0x60,
    0x01, 0x6f, 0x01, 0x6f, 0x60, 0x01, 0x6f, 0x00, 0x60, 0x00, 0x01, 0x6f,
    0x60, 0x02, 0x7f, 0x6f, 0x00, 0x60, 0x01, 0x7f, 0x01, 0x6f, 0x02, 0x06,
    0x01, 0x00, 0x01, 0x66, 0x00, 0x00, 0x03, 0x06, 0x05, 0x01, 0x02, 0x03,
    0x04, 0x00, 0x04, 0x04, 0x01, 0x6f, 0x00, 0x0a, 0x06, 0x06, 0x01, 0x6f,
    0x01, 0xd0, 0x6f, 0x0b, 0x07, 0x50, 0x07, 0x06, 0x67, 0x6c, 0x6f, 0x62,
    0x61, 0x6c, 0x03, 0x00, 0x05, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x01, 0x00,
    0x0a, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x2e, 0x73, 0x65, 0x74, 0x00,
    0x01, 0x0a, 0x67, 0x6c, 0x6f, 0x62, 0x61, 0x6c, 0x2e, 0x67, 0x65, 0x74,
    0x00, 0x02, 0x09, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x73, 0x65, 0x74,
    0x00, 0x03, 0x09, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x2e, 0x67, 0x65, 0x74,
    0x00, 0x04, 0x09, 0x66, 0x75, 0x6e, 0x63, 0x2e, 0x63, 0x61, 0x6c, 0x6c,
    0x00, 0x05, 0x0a, 0x24, 0x05, 0x06, 0x00, 0x20, 0x00, 0x24, 0x00, 0x0b,
    0x04, 0x00, 0x23, 0x00, 0x0b, 0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x26,
    0x00, 0x0b, 0x06, 0x00, 0x20, 0x00, 0x25, 0x00, 0x0b, 0x06, 0x00, 0x20,
    0x00, 0x10, 0x00, 0x0b};

wasm_trap_t *hostrefEchoCallback(const wasm_val_vec_t *args,
                                 wasm_val_vec_t *results) {
  wasm_val_copy(&results->data[0], &args->data[0]);
  return nullptr;
}

namespace {
// Wrap upstream's check(actual, expected): assert that two refs refer to the
// same host object, then drop actual's ownership.
void checkSameRef(wasm_ref_t *actual, const wasm_ref_t *expected) {
  if (actual == nullptr || expected == nullptr) {
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_TRUE(wasm_ref_same(actual, expected));
  }
  if (actual) {
    wasm_ref_delete(actual);
  }
}
wasm_ref_t *callExternRefGetter(const wasm_func_t *fn) {
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {0, nullptr};
  wasm_val_vec_t res = {1, &out};
  EXPECT_EQ(nullptr,
            wasm_func_call(const_cast<wasm_func_t *>(fn), &args, &res));
  return out.of.ref;
}
void callExternRefSetter(const wasm_func_t *fn, wasm_ref_t *ref) {
  wasm_val_t v{};
  v.kind = WASM_EXTERNREF;
  v.of.ref = ref;
  wasm_val_vec_t args = {1, &v};
  wasm_val_vec_t res = {0, nullptr};
  EXPECT_EQ(nullptr,
            wasm_func_call(const_cast<wasm_func_t *>(fn), &args, &res));
}
wasm_ref_t *callIndexedExternRefGetter(const wasm_func_t *fn, int32_t i) {
  wasm_val_t arg = i32Val(i);
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {1, &arg};
  wasm_val_vec_t res = {1, &out};
  EXPECT_EQ(nullptr,
            wasm_func_call(const_cast<wasm_func_t *>(fn), &args, &res));
  return out.of.ref;
}
void callIndexedExternRefSetter(const wasm_func_t *fn, int32_t i,
                                wasm_ref_t *ref) {
  wasm_val_t arg_buf[2];
  arg_buf[0] = i32Val(i);
  arg_buf[1] = wasm_val_t{};
  arg_buf[1].kind = WASM_EXTERNREF;
  arg_buf[1].of.ref = ref;
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t res = {0, nullptr};
  EXPECT_EQ(nullptr,
            wasm_func_call(const_cast<wasm_func_t *>(fn), &args, &res));
}
wasm_ref_t *callRefEcho(const wasm_func_t *fn, wasm_ref_t *ref) {
  wasm_val_t arg{};
  arg.kind = WASM_EXTERNREF;
  arg.of.ref = ref;
  wasm_val_t out = initVal();
  wasm_val_vec_t args = {1, &arg};
  wasm_val_vec_t res = {1, &out};
  EXPECT_EQ(nullptr,
            wasm_func_call(const_cast<wasm_func_t *>(fn), &args, &res));
  return out.of.ref;
}
} // namespace

TEST(APIWasmCExampleTest, Hostref) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, HostrefWasm.size());
  std::copy(HostrefWasm.begin(), HostrefWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *callback_type = wasm_functype_new_1_1(
      wasm_valtype_new(WASM_EXTERNREF), wasm_valtype_new(WASM_EXTERNREF));
  wasm_func_t *callback_func =
      wasm_func_new(store, callback_type, hostrefEchoCallback);
  wasm_functype_delete(callback_type);

  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(callback_func)};
  wasm_extern_vec_t imports_vec = {1, ext_imports};
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, nullptr);
  ASSERT_NE(nullptr, instance);
  wasm_func_delete(callback_func);
  wasm_module_delete(module);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(7U, exports.size);
  wasm_global_t *global = wasm_extern_as_global(exports.data[0]);
  wasm_table_t *table = wasm_extern_as_table(exports.data[1]);
  wasm_func_t *global_set = wasm_extern_as_func(exports.data[2]);
  wasm_func_t *global_get = wasm_extern_as_func(exports.data[3]);
  wasm_func_t *table_set = wasm_extern_as_func(exports.data[4]);
  wasm_func_t *table_get = wasm_extern_as_func(exports.data[5]);
  wasm_func_t *func_call = wasm_extern_as_func(exports.data[6]);

  // Two host references, distinguished by host_info payload.
  wasm_ref_t *host1 = wasm_foreign_as_ref(wasm_foreign_new(store));
  wasm_ref_t *host2 = wasm_foreign_as_ref(wasm_foreign_new(store));
  wasm_ref_set_host_info(host1, reinterpret_cast<void *>(1));
  wasm_ref_set_host_info(host2, reinterpret_cast<void *>(2));

  // Sanity: ref_copy returns a handle that compares equal to the original.
  checkSameRef(nullptr, nullptr);
  checkSameRef(wasm_ref_copy(host1), host1);
  checkSameRef(wasm_ref_copy(host2), host2);

  // Round-trip through a wasm_val_t.
  wasm_val_t val{};
  val.kind = WASM_EXTERNREF;
  val.of.ref = wasm_ref_copy(host1);
  checkSameRef(wasm_ref_copy(val.of.ref), host1);
  wasm_val_delete(&val);

  // Global: get returns null initially; set/get round-trips host1, host2,
  // null. Then mirror via the host wasm_global_get/set API.
  checkSameRef(callExternRefGetter(global_get), nullptr);
  callExternRefSetter(global_set, host1);
  checkSameRef(callExternRefGetter(global_get), host1);
  callExternRefSetter(global_set, host2);
  checkSameRef(callExternRefGetter(global_get), host2);
  callExternRefSetter(global_set, nullptr);
  checkSameRef(callExternRefGetter(global_get), nullptr);

  wasm_global_get(global, &val);
  EXPECT_EQ(WASM_EXTERNREF, val.kind);
  EXPECT_EQ(nullptr, val.of.ref);
  val.of.ref = host2;
  wasm_global_set(global, &val);
  checkSameRef(callExternRefGetter(global_get), host2);
  wasm_global_get(global, &val);
  EXPECT_EQ(WASM_EXTERNREF, val.kind);
  if (val.of.ref) {
    EXPECT_TRUE(wasm_ref_same(val.of.ref, host2));
    wasm_val_delete(&val);
  }

  // Table: initial slots null; set/get round-trips; null clears a slot.
  checkSameRef(callIndexedExternRefGetter(table_get, 0), nullptr);
  checkSameRef(callIndexedExternRefGetter(table_get, 1), nullptr);
  callIndexedExternRefSetter(table_set, 0, host1);
  callIndexedExternRefSetter(table_set, 1, host2);
  checkSameRef(callIndexedExternRefGetter(table_get, 0), host1);
  checkSameRef(callIndexedExternRefGetter(table_get, 1), host2);
  callIndexedExternRefSetter(table_set, 0, nullptr);
  checkSameRef(callIndexedExternRefGetter(table_get, 0), nullptr);

  // Host-side wasm_table_set on an externref table.
  EXPECT_EQ(nullptr, wasm_table_get(table, 2));
  wasm_table_set(table, 2, host1);
  checkSameRef(callIndexedExternRefGetter(table_get, 2), host1);
  wasm_ref_t *direct = wasm_table_get(table, 2);
  if (direct) {
    EXPECT_TRUE(wasm_ref_same(direct, host1));
    wasm_ref_delete(direct);
  }

  // func.call invokes the host callback that echoes its arg.
  checkSameRef(callRefEcho(func_call, nullptr), nullptr);
  checkSameRef(callRefEcho(func_call, host1), host1);
  checkSameRef(callRefEcho(func_call, host2), host2);

  // Upstream's pattern: delete the table/global before the host refs, so the
  // runtime no longer holds borrowed pointers to host1/host2.
  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_ref_delete(host1);
  wasm_ref_delete(host2);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

wasm::own<wasm::Trap> hostrefCppEchoCallback(const wasm::vec<wasm::Val> &args,
                                             wasm::vec<wasm::Val> &results) {
  results[0] = args[0].copy();
  return nullptr;
}

namespace {
void checkSameRefCpp(wasm::own<wasm::Ref> actual, const wasm::Ref *expected) {
  if (actual.get() == nullptr || expected == nullptr) {
    EXPECT_EQ(expected, actual.get());
  } else {
    EXPECT_TRUE(actual->same(expected));
  }
}
wasm::own<wasm::Ref> callExternRefGetterCpp(const wasm::Func *fn) {
  auto args = wasm::vec<wasm::Val>::make();
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, res));
  return res[0].release_ref();
}
void callExternRefSetterCpp(const wasm::Func *fn, wasm::Ref *ref) {
  auto args = wasm::vec<wasm::Val>::make(
      wasm::Val(ref ? ref->copy() : wasm::own<wasm::Ref>{}));
  auto res = wasm::vec<wasm::Val>::make();
  EXPECT_EQ(nullptr, fn->call(args, res));
}
wasm::own<wasm::Ref> callIndexedExternRefGetterCpp(const wasm::Func *fn,
                                                   int32_t i) {
  auto args = wasm::vec<wasm::Val>::make(wasm::Val::i32(i));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, res));
  return res[0].release_ref();
}
void callIndexedExternRefSetterCpp(const wasm::Func *fn, int32_t i,
                                   wasm::Ref *ref) {
  auto args = wasm::vec<wasm::Val>::make(
      wasm::Val::i32(i), wasm::Val(ref ? ref->copy() : wasm::own<wasm::Ref>{}));
  auto res = wasm::vec<wasm::Val>::make();
  EXPECT_EQ(nullptr, fn->call(args, res));
}
wasm::own<wasm::Ref> callRefEchoCpp(const wasm::Func *fn, wasm::Ref *ref) {
  auto args = wasm::vec<wasm::Val>::make(
      wasm::Val(ref ? ref->copy() : wasm::own<wasm::Ref>{}));
  auto res = wasm::vec<wasm::Val>::make_uninitialized(1);
  EXPECT_EQ(nullptr, fn->call(args, res));
  return res[0].release_ref();
}
} // namespace

TEST(APIWasmCExampleTest, HostrefCpp) {
  auto engine = wasm::Engine::make();
  auto store = wasm::Store::make(engine.get());

  auto binary = wasm::vec<byte_t>::make_uninitialized(HostrefWasm.size());
  std::copy(HostrefWasm.begin(), HostrefWasm.end(), binary.get());
  auto module = wasm::Module::make(store.get(), binary);
  ASSERT_NE(nullptr, module);

  auto callback_type =
      wasm::FuncType::make(wasm::ownvec<wasm::ValType>::make(
                               wasm::ValType::make(wasm::ValKind::EXTERNREF)),
                           wasm::ownvec<wasm::ValType>::make(
                               wasm::ValType::make(wasm::ValKind::EXTERNREF)));
  auto callback_func = wasm::Func::make(store.get(), callback_type.get(),
                                        hostrefCppEchoCallback);

  auto imports = wasm::vec<wasm::Extern *>::make(callback_func.get());
  auto instance = wasm::Instance::make(store.get(), module.get(), imports);
  ASSERT_NE(nullptr, instance);

  auto exports = instance->exports();
  ASSERT_EQ(7U, exports.size());
  auto *global = exports[0]->global();
  auto *table = exports[1]->table();
  auto *global_set = exports[2]->func();
  auto *global_get = exports[3]->func();
  auto *table_set = exports[4]->func();
  auto *table_get = exports[5]->func();
  auto *func_call = exports[6]->func();

  auto host1 = wasm::Foreign::make(store.get());
  auto host2 = wasm::Foreign::make(store.get());
  host1->set_host_info(reinterpret_cast<void *>(1));
  host2->set_host_info(reinterpret_cast<void *>(2));

  // Sanity: ref->copy() returns a handle equal to the original.
  EXPECT_TRUE(host1->copy()->same(host1.get()));
  EXPECT_TRUE(host2->copy()->same(host2.get()));

  // Global: initial null, set/get round-trip, host-API mirror.
  checkSameRefCpp(callExternRefGetterCpp(global_get), nullptr);
  callExternRefSetterCpp(global_set, host1.get());
  checkSameRefCpp(callExternRefGetterCpp(global_get), host1.get());
  callExternRefSetterCpp(global_set, host2.get());
  checkSameRefCpp(callExternRefGetterCpp(global_get), host2.get());
  callExternRefSetterCpp(global_set, nullptr);
  checkSameRefCpp(callExternRefGetterCpp(global_get), nullptr);

  auto val = global->get();
  EXPECT_EQ(wasm::ValKind::EXTERNREF, val.kind());
  EXPECT_EQ(nullptr, val.ref());
  global->set(wasm::Val(host2->copy()));
  checkSameRefCpp(callExternRefGetterCpp(global_get), host2.get());

  // Table: round-trip through wasm + host API.
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 0), nullptr);
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 1), nullptr);
  callIndexedExternRefSetterCpp(table_set, 0, host1.get());
  callIndexedExternRefSetterCpp(table_set, 1, host2.get());
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 0), host1.get());
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 1), host2.get());
  callIndexedExternRefSetterCpp(table_set, 0, nullptr);
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 0), nullptr);

  EXPECT_EQ(nullptr, table->get(2));
  table->set(2, host1.get());
  checkSameRefCpp(callIndexedExternRefGetterCpp(table_get, 2), host1.get());
  auto direct = table->get(2);
  if (direct) {
    EXPECT_TRUE(direct->same(host1.get()));
  }

  // func.call echoes its arg.
  checkSameRefCpp(callRefEchoCpp(func_call, nullptr), nullptr);
  checkSameRefCpp(callRefEchoCpp(func_call, host1.get()), host1.get());
  checkSameRefCpp(callRefEchoCpp(func_call, host2.get()), host2.get());
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
