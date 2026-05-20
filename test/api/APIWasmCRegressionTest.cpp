// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
//
// Regression tests for WasmEdge's wasm-c-api implementation.

#include "wasm.h"

#include "gtest/gtest.h"

#include <array>

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

// Test fixture for ReuseHostFuncAsImport (hello.c module): import a host
// "hello" function and export a "run" that calls it.
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

// Regression test: the same host wasm_func_t must be reusable as an import to
// multiple instances (spec semantics — imports are borrowed, not consumed).
TEST(APIWasmCRegressionTest, ReuseHostFuncAsImport) {
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

  // First instantiation: pass hello_func, call run, expect callback fires.
  wasm_extern_t *imports1[] = {wasm_func_as_extern(hello_func)};
  wasm_extern_vec_t imports1_vec = {1, imports1};
  wasm_trap_t *trap1 = nullptr;
  wasm_instance_t *inst1 =
      wasm_instance_new(store, module, &imports1_vec, &trap1);
  ASSERT_NE(nullptr, inst1);
  wasm_extern_vec_t exports1;
  wasm_instance_exports(inst1, &exports1);
  wasm_val_vec_t empty_args = {0, nullptr};
  wasm_val_vec_t empty_res = {0, nullptr};
  ASSERT_EQ(nullptr, wasm_func_call(wasm_extern_as_func(exports1.data[0]),
                                    &empty_args, &empty_res));
  EXPECT_EQ(1, HelloCallCount);

  // Second instantiation: reuse the SAME hello_func. Before the fix, the
  // first instantiation would have stripped hello_func->inst.
  wasm_extern_t *imports2[] = {wasm_func_as_extern(hello_func)};
  wasm_extern_vec_t imports2_vec = {1, imports2};
  wasm_trap_t *trap2 = nullptr;
  wasm_instance_t *inst2 =
      wasm_instance_new(store, module, &imports2_vec, &trap2);
  ASSERT_NE(nullptr, inst2);
  wasm_extern_vec_t exports2;
  wasm_instance_exports(inst2, &exports2);
  ASSERT_EQ(nullptr, wasm_func_call(wasm_extern_as_func(exports2.data[0]),
                                    &empty_args, &empty_res));
  EXPECT_EQ(2, HelloCallCount);

  wasm_extern_vec_delete(&exports2);
  wasm_extern_vec_delete(&exports1);
  wasm_instance_delete(inst2);
  wasm_instance_delete(inst1);
  wasm_func_delete(hello_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

// Regression test: a native-wasm function exported from one instance must be
// usable as an import to another instance. This exercises the alias path for
// non-host targets — the wrapper dispatches via Executor::invoke at call
// time.
//
// Provider:
//   (module (func (export "add") (param i32 i32) (result i32)
//     (i32.add (local.get 0) (local.get 1))))
const std::array<uint8_t, 41> PassthroughProviderWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
    0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
    0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
    0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b};

// Consumer:
//   (module
//     (import "" "add" (func $add (param i32 i32) (result i32)))
//     (func (export "call_add") (param i32 i32) (result i32)
//       (call $add (local.get 0) (local.get 1))))
const std::array<uint8_t, 57> PassthroughConsumerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x08, 0x01, 0x00, 0x03, 0x61, 0x64,
    0x64, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0c, 0x01, 0x08, 0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x61, 0x64, 0x64, 0x00, 0x01, 0x0a, 0x0a, 0x01,
    0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0b};

TEST(APIWasmCRegressionTest, NativeFuncAsImport) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // Instantiate provider; pick up its exported "add".
  wasm_byte_vec_t prov_bin;
  wasm_byte_vec_new_uninitialized(&prov_bin, PassthroughProviderWasm.size());
  std::copy(PassthroughProviderWasm.begin(), PassthroughProviderWasm.end(),
            prov_bin.data);
  wasm_module_t *prov_mod = wasm_module_new(store, &prov_bin);
  wasm_byte_vec_delete(&prov_bin);
  ASSERT_NE(nullptr, prov_mod);

  wasm_extern_vec_t no_imports = {0, nullptr};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *prov_inst =
      wasm_instance_new(store, prov_mod, &no_imports, &trap);
  ASSERT_NE(nullptr, prov_inst);

  wasm_extern_vec_t prov_exports;
  wasm_instance_exports(prov_inst, &prov_exports);
  ASSERT_EQ(1U, prov_exports.size);
  wasm_func_t *add_fn = wasm_extern_as_func(prov_exports.data[0]);
  ASSERT_NE(nullptr, add_fn);

  // Instantiate consumer, passing the provider's "add" as its import.
  wasm_byte_vec_t cons_bin;
  wasm_byte_vec_new_uninitialized(&cons_bin, PassthroughConsumerWasm.size());
  std::copy(PassthroughConsumerWasm.begin(), PassthroughConsumerWasm.end(),
            cons_bin.data);
  wasm_module_t *cons_mod = wasm_module_new(store, &cons_bin);
  wasm_byte_vec_delete(&cons_bin);
  ASSERT_NE(nullptr, cons_mod);

  wasm_extern_t *imports[] = {wasm_func_as_extern(add_fn)};
  wasm_extern_vec_t imports_vec = {1, imports};
  wasm_instance_t *cons_inst =
      wasm_instance_new(store, cons_mod, &imports_vec, &trap);
  ASSERT_NE(nullptr, cons_inst);

  wasm_extern_vec_t cons_exports;
  wasm_instance_exports(cons_inst, &cons_exports);
  wasm_func_t *call_add = wasm_extern_as_func(cons_exports.data[0]);

  // call_add(3, 4) should reach the provider's add and return 7.
  wasm_val_t arg_buf[2] = {i32Val(3), i32Val(4)};
  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t results = {1, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(call_add, &args, &results));
  EXPECT_EQ(7, res_buf[0].of.i32);

  wasm_extern_vec_delete(&cons_exports);
  wasm_instance_delete(cons_inst);
  wasm_module_delete(cons_mod);
  wasm_extern_vec_delete(&prov_exports);
  wasm_instance_delete(prov_inst);
  wasm_module_delete(prov_mod);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

// Regression test: a wasm module that imports from non-"" module names must
// resolve correctly. Spec semantics: imports are a flat positional vec, but
// the engine must honor whatever (module-name, ext-name) pairs the wasm
// module itself declares.
//
// (module
//   (import "env" "log" (func $log (param i32)))
//   (import "math" "add" (func $add (param i32 i32) (result i32)))
//   (func (export "run") (param i32 i32) (result i32)
//     (call $log (local.get 0))
//     (call $add (local.get 0) (local.get 1))))
const std::array<uint8_t, 74> NamedImportsWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x02,
    0x60, 0x01, 0x7f, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02,
    0x16, 0x02, 0x03, 0x65, 0x6e, 0x76, 0x03, 0x6c, 0x6f, 0x67, 0x00,
    0x00, 0x04, 0x6d, 0x61, 0x74, 0x68, 0x03, 0x61, 0x64, 0x64, 0x00,
    0x01, 0x03, 0x02, 0x01, 0x01, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75,
    0x6e, 0x00, 0x02, 0x0a, 0x0e, 0x01, 0x0c, 0x00, 0x20, 0x00, 0x10,
    0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x01, 0x0b};

int LogCallCount = 0;
int32_t LogLastArg = 0;
wasm_trap_t *logCallback(const wasm_val_vec_t *args, wasm_val_vec_t *) {
  LogCallCount += 1;
  LogLastArg = args->data[0].of.i32;
  return nullptr;
}
wasm_trap_t *addCallback(const wasm_val_vec_t *args, wasm_val_vec_t *results) {
  results->data[0].kind = WASM_I32;
  results->data[0].of.i32 = args->data[0].of.i32 + args->data[1].of.i32;
  return nullptr;
}

TEST(APIWasmCRegressionTest, NamedModuleImports) {
  LogCallCount = 0;

  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, NamedImportsWasm.size());
  std::copy(NamedImportsWasm.begin(), NamedImportsWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *log_type = wasm_functype_new_1_0(wasm_valtype_new_i32());
  wasm_func_t *log_func = wasm_func_new(store, log_type, logCallback);
  wasm_functype_delete(log_type);

  wasm_functype_t *add_type = wasm_functype_new_2_1(
      wasm_valtype_new_i32(), wasm_valtype_new_i32(), wasm_valtype_new_i32());
  wasm_func_t *add_func = wasm_func_new(store, add_type, addCallback);
  wasm_functype_delete(add_type);

  // Positional vec: imports[0] -> ("env","log"), imports[1] -> ("math","add").
  wasm_extern_t *ext_imports[] = {wasm_func_as_extern(log_func),
                                  wasm_func_as_extern(add_func)};
  wasm_extern_vec_t imports_vec = {2, ext_imports};
  wasm_trap_t *trap = nullptr;
  wasm_instance_t *instance =
      wasm_instance_new(store, module, &imports_vec, &trap);
  ASSERT_NE(nullptr, instance);
  ASSERT_EQ(nullptr, trap);

  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  ASSERT_EQ(1U, exports.size);
  wasm_func_t *run = wasm_extern_as_func(exports.data[0]);

  wasm_val_t arg_buf[2] = {i32Val(5), i32Val(7)};
  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t args = {2, arg_buf};
  wasm_val_vec_t results = {1, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(run, &args, &results));
  EXPECT_EQ(12, res_buf[0].of.i32);
  EXPECT_EQ(1, LogCallCount);
  EXPECT_EQ(5, LogLastArg);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(add_func);
  wasm_func_delete(log_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

// Reentrancy: a host callback invoked from wasm calls wasm_func_call on
// another wasm export.
//
// (module
//   (import "" "cb" (func $cb))
//   (func $inner (export "inner") (result i32) (i32.const 42))
//   (func (export "outer") (result i32)
//     (call $cb)
//     (call $inner)))
const std::array<uint8_t, 66> ReentrantWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
    0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x02, 0x07, 0x01, 0x00,
    0x02, 0x63, 0x62, 0x00, 0x00, 0x03, 0x03, 0x02, 0x01, 0x01, 0x07,
    0x11, 0x02, 0x05, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x00, 0x01, 0x05,
    0x6f, 0x75, 0x74, 0x65, 0x72, 0x00, 0x02, 0x0a, 0x0d, 0x02, 0x04,
    0x00, 0x41, 0x2a, 0x0b, 0x06, 0x00, 0x10, 0x00, 0x10, 0x01, 0x0b};

wasm_func_t *ReentrantInner = nullptr;
int ReentrantCbCount = 0;
int32_t ReentrantInnerResult = 0;
wasm_trap_t *reentrantCallback(const wasm_val_vec_t *, wasm_val_vec_t *) {
  ReentrantCbCount += 1;
  wasm_val_t buf[1] = {initVal()};
  wasm_val_vec_t no_args = {0, nullptr};
  wasm_val_vec_t res = {1, buf};
  if (wasm_func_call(ReentrantInner, &no_args, &res)) {
    return nullptr;
  }
  ReentrantInnerResult = buf[0].of.i32;
  return nullptr;
}

TEST(APIWasmCRegressionTest, HostfuncReentrancy) {
  ReentrantInner = nullptr;
  ReentrantCbCount = 0;
  ReentrantInnerResult = 0;

  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t binary;
  wasm_byte_vec_new_uninitialized(&binary, ReentrantWasm.size());
  std::copy(ReentrantWasm.begin(), ReentrantWasm.end(), binary.data);
  wasm_module_t *module = wasm_module_new(store, &binary);
  wasm_byte_vec_delete(&binary);
  ASSERT_NE(nullptr, module);

  wasm_functype_t *cb_type = wasm_functype_new_0_0();
  wasm_func_t *cb_func = wasm_func_new(store, cb_type, reentrantCallback);
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
  ReentrantInner = wasm_extern_as_func(exports.data[0]);
  wasm_func_t *outer = wasm_extern_as_func(exports.data[1]);

  wasm_val_t res_buf[1] = {initVal()};
  wasm_val_vec_t no_args = {0, nullptr};
  wasm_val_vec_t results = {1, res_buf};
  ASSERT_EQ(nullptr, wasm_func_call(outer, &no_args, &results));
  EXPECT_EQ(42, res_buf[0].of.i32);
  EXPECT_EQ(1, ReentrantCbCount);
  EXPECT_EQ(42, ReentrantInnerResult);

  wasm_extern_vec_delete(&exports);
  wasm_instance_delete(instance);
  wasm_func_delete(cb_func);
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
