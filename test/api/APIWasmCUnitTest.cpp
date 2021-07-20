// SPDX-License-Identifier: Apache-2.0
#include "wasm.h"

#include "gtest/gtest.h"

namespace {

/// Helper macro for the WASM_DECLARE_OWN_IMPL test.
#define WASM_DECLARE_OWN_TEST(name, obj)                                       \
  wasm_##name##_delete(nullptr);                                               \
  wasm_##name##_delete(obj);

/// Helper macro for the WASM_DECLARE_VEC_IMPL test.
#define WASM_DECLARE_VEC_TEST(name, size, vals, out, copy)                     \
  wasm_##name##_vec_new_empty(nullptr);                                        \
  wasm_##name##_vec_new_empty(&out);                                           \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new_uninitialized(nullptr, size);                          \
  wasm_##name##_vec_new_uninitialized(&out, 0);                                \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new_uninitialized(&out, size);                             \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new(nullptr, size, vals);                                  \
  wasm_##name##_vec_new(&out, 0, nullptr);                                     \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new(&out, size, vals);                                     \
  wasm_##name##_vec_new_empty(&copy);                                          \
  wasm_##name##_vec_copy(nullptr, nullptr);                                    \
  wasm_##name##_vec_copy(nullptr, &out);                                       \
  wasm_##name##_vec_copy(&copy, nullptr);                                      \
  wasm_##name##_vec_copy(&copy, &out);

/// Helper macro for the deletion function of WASM_DECLARE_VEC_IMPL test.
#define WASM_DECLARE_VEC_DEL_TEST(name, out, copy)                             \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_delete(&copy);                                             \
  wasm_##name##_vec_delete(&out);

/// Helper macro for the WASM_DECLARE_TYPE_IMPL test.
#define WASM_DECLARE_TYPE_TEST(name, size, vals, out, copy)                    \
  wasm_##name##_t *dup_##name = wasm_##name##_copy(vals[0]);                   \
  WASM_DECLARE_OWN_TEST(name, dup_##name)                                      \
  dup_##name = wasm_##name##_copy(nullptr);                                    \
  WASM_DECLARE_VEC_TEST(name, size, vals, out, copy)

TEST(APIWasmCTest, Byte) {
  wasm_byte_t bytes[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  wasm_byte_vec_t out, copy;
  WASM_DECLARE_VEC_TEST(byte, 10, bytes, out, copy)
  WASM_DECLARE_VEC_DEL_TEST(byte, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Store) {
  wasm_config_t *conf = wasm_config_new();
  WASM_DECLARE_OWN_TEST(config, conf)
  conf = wasm_config_new();
  wasm_engine_t *engine = wasm_engine_new();
  WASM_DECLARE_OWN_TEST(engine, engine)
  engine = wasm_engine_new_with_config(conf);
  wasm_store_t *store = wasm_store_new(nullptr);
  WASM_DECLARE_OWN_TEST(store, store)
  store = wasm_store_new(engine);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ValType) {
  wasm_valtype_t *valtypes[6] = {
      wasm_valtype_new(WASM_I32),    wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32),    wasm_valtype_new(WASM_F64),
      wasm_valtype_new(WASM_ANYREF), wasm_valtype_new(WASM_FUNCREF)};
  EXPECT_EQ(WASM_ANYREF, wasm_valtype_kind(valtypes[4]));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(nullptr));
  wasm_valtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(valtype, 6, valtypes, out, copy)
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(copy.data[0]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(copy.data[1]));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(copy.data[2]));
  EXPECT_EQ(WASM_F64, wasm_valtype_kind(copy.data[3]));
  EXPECT_EQ(WASM_ANYREF, wasm_valtype_kind(copy.data[4]));
  EXPECT_EQ(WASM_FUNCREF, wasm_valtype_kind(copy.data[5]));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(out.data[0]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(out.data[1]));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(out.data[2]));
  EXPECT_EQ(WASM_F64, wasm_valtype_kind(out.data[3]));
  EXPECT_EQ(WASM_ANYREF, wasm_valtype_kind(out.data[4]));
  EXPECT_EQ(WASM_FUNCREF, wasm_valtype_kind(out.data[5]));
  WASM_DECLARE_VEC_DEL_TEST(valtype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, FuncType) {
  wasm_valtype_vec_t vt[8];
  wasm_valtype_t *valtypes[4] = {
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32), wasm_valtype_new(WASM_F64)};
  wasm_valtype_vec_new(&vt[0], 4, valtypes);
  for (uint32_t i = 1; i < 8; i++) {
    wasm_valtype_vec_copy(&vt[i], &vt[0]);
  }
  wasm_functype_t *functypes[4] = {
      wasm_functype_new(&vt[0], &vt[1]), wasm_functype_new(&vt[2], &vt[3]),
      wasm_functype_new(&vt[4], &vt[5]), wasm_functype_new(&vt[6], &vt[7])};
  EXPECT_NE(nullptr, wasm_functype_params(functypes[0]));
  EXPECT_EQ(nullptr, wasm_functype_params(nullptr));
  EXPECT_NE(nullptr, wasm_functype_results(functypes[0]));
  EXPECT_EQ(nullptr, wasm_functype_results(nullptr));
  wasm_functype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(functype, 4, functypes, out, copy)
  EXPECT_EQ(WASM_F32,
            wasm_valtype_kind(wasm_functype_params(copy.data[0])->data[2]));
  EXPECT_EQ(WASM_F32,
            wasm_valtype_kind(wasm_functype_params(copy.data[1])->data[2]));
  EXPECT_EQ(WASM_F32,
            wasm_valtype_kind(wasm_functype_params(copy.data[2])->data[2]));
  EXPECT_EQ(WASM_F32,
            wasm_valtype_kind(wasm_functype_params(copy.data[3])->data[2]));
  EXPECT_EQ(WASM_I64,
            wasm_valtype_kind(wasm_functype_results(copy.data[0])->data[1]));
  EXPECT_EQ(WASM_I64,
            wasm_valtype_kind(wasm_functype_results(copy.data[1])->data[1]));
  EXPECT_EQ(WASM_I64,
            wasm_valtype_kind(wasm_functype_results(copy.data[2])->data[1]));
  EXPECT_EQ(WASM_I64,
            wasm_valtype_kind(wasm_functype_results(copy.data[3])->data[1]));
  WASM_DECLARE_VEC_DEL_TEST(functype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, GlobalType) {
  wasm_valtype_t *valtypes[6] = {
      wasm_valtype_new(WASM_I32),    wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32),    wasm_valtype_new(WASM_F64),
      wasm_valtype_new(WASM_ANYREF), wasm_valtype_new(WASM_FUNCREF)};
  wasm_globaltype_t *globaltypes[6] = {
      wasm_globaltype_new(valtypes[0], WASM_CONST),
      wasm_globaltype_new(valtypes[1], WASM_VAR),
      wasm_globaltype_new(valtypes[2], WASM_CONST),
      wasm_globaltype_new(valtypes[3], WASM_VAR),
      wasm_globaltype_new(valtypes[4], WASM_CONST),
      wasm_globaltype_new(valtypes[5], WASM_VAR)};
  EXPECT_NE(nullptr, wasm_globaltype_content(globaltypes[0]));
  EXPECT_EQ(nullptr, wasm_globaltype_content(nullptr));
  EXPECT_EQ(WASM_VAR, wasm_globaltype_mutability(globaltypes[1]));
  EXPECT_EQ(WASM_CONST, wasm_globaltype_mutability(nullptr));
  wasm_globaltype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(globaltype, 6, globaltypes, out, copy)
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(wasm_globaltype_content(copy.data[0])));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(wasm_globaltype_content(copy.data[1])));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(wasm_globaltype_content(copy.data[2])));
  EXPECT_EQ(WASM_F64, wasm_valtype_kind(wasm_globaltype_content(copy.data[3])));
  EXPECT_EQ(WASM_ANYREF,
            wasm_valtype_kind(wasm_globaltype_content(copy.data[4])));
  EXPECT_EQ(WASM_FUNCREF,
            wasm_valtype_kind(wasm_globaltype_content(copy.data[5])));
  EXPECT_EQ(WASM_CONST, wasm_globaltype_mutability(copy.data[0]));
  EXPECT_EQ(WASM_VAR, wasm_globaltype_mutability(copy.data[1]));
  EXPECT_EQ(WASM_CONST, wasm_globaltype_mutability(copy.data[2]));
  EXPECT_EQ(WASM_VAR, wasm_globaltype_mutability(copy.data[3]));
  EXPECT_EQ(WASM_CONST, wasm_globaltype_mutability(copy.data[4]));
  EXPECT_EQ(WASM_VAR, wasm_globaltype_mutability(copy.data[5]));
  WASM_DECLARE_VEC_DEL_TEST(globaltype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, TableType) {
  wasm_valtype_t *valtypes[3] = {wasm_valtype_new(WASM_ANYREF),
                                 wasm_valtype_new(WASM_FUNCREF),
                                 wasm_valtype_new(WASM_FUNCREF)};
  wasm_limits_t limits = {.min = 10, .max = 20};
  wasm_tabletype_t *tabletypes[3] = {
      wasm_tabletype_new(valtypes[0], &limits),
      wasm_tabletype_new(valtypes[1], &limits),
      wasm_tabletype_new(valtypes[2], &limits),
  };
  EXPECT_NE(nullptr, wasm_tabletype_element(tabletypes[0]));
  EXPECT_EQ(nullptr, wasm_tabletype_element(nullptr));
  EXPECT_NE(nullptr, wasm_tabletype_limits(tabletypes[0]));
  EXPECT_EQ(nullptr, wasm_tabletype_limits(nullptr));
  wasm_tabletype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(tabletype, 3, tabletypes, out, copy)
  EXPECT_EQ(WASM_ANYREF,
            wasm_valtype_kind(wasm_tabletype_element(copy.data[0])));
  EXPECT_EQ(WASM_FUNCREF,
            wasm_valtype_kind(wasm_tabletype_element(copy.data[1])));
  EXPECT_EQ(WASM_FUNCREF,
            wasm_valtype_kind(wasm_tabletype_element(copy.data[2])));
  EXPECT_EQ(10U, wasm_tabletype_limits(copy.data[0])->min);
  EXPECT_EQ(10U, wasm_tabletype_limits(copy.data[1])->min);
  EXPECT_EQ(10U, wasm_tabletype_limits(copy.data[2])->min);
  EXPECT_EQ(20U, wasm_tabletype_limits(copy.data[0])->max);
  EXPECT_EQ(20U, wasm_tabletype_limits(copy.data[1])->max);
  EXPECT_EQ(20U, wasm_tabletype_limits(copy.data[2])->max);
  WASM_DECLARE_VEC_DEL_TEST(tabletype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, MemoryType) {
  wasm_limits_t limits = {.min = 10, .max = 20};
  wasm_memorytype_t *memorytypes[3] = {wasm_memorytype_new(&limits),
                                       wasm_memorytype_new(&limits),
                                       wasm_memorytype_new(&limits)};
  EXPECT_NE(nullptr, wasm_memorytype_limits(memorytypes[0]));
  EXPECT_EQ(nullptr, wasm_memorytype_limits(nullptr));
  wasm_memorytype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(memorytype, 3, memorytypes, out, copy)
  EXPECT_EQ(10U, wasm_memorytype_limits(copy.data[0])->min);
  EXPECT_EQ(10U, wasm_memorytype_limits(copy.data[1])->min);
  EXPECT_EQ(10U, wasm_memorytype_limits(copy.data[2])->min);
  EXPECT_EQ(20U, wasm_memorytype_limits(copy.data[0])->max);
  EXPECT_EQ(20U, wasm_memorytype_limits(copy.data[1])->max);
  EXPECT_EQ(20U, wasm_memorytype_limits(copy.data[2])->max);
  WASM_DECLARE_VEC_DEL_TEST(memorytype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ExternType) {
  /// Create functype
  wasm_valtype_vec_t params, results;
  wasm_valtype_t *valtypes[4] = {
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32), wasm_valtype_new(WASM_F64)};
  wasm_valtype_vec_new(&params, 4, valtypes);
  wasm_valtype_vec_copy(&results, &params);
  wasm_functype_t *functype = wasm_functype_new(&params, &results);
  /// Create globaltype
  wasm_valtype_t *valtype = wasm_valtype_new(WASM_I64);
  wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_VAR);
  /// Create tabletype
  wasm_limits_t limits = {.min = 10, .max = 20};
  valtype = wasm_valtype_new(WASM_ANYREF);
  wasm_tabletype_t *tabletype = wasm_tabletype_new(valtype, &limits);
  /// Create memorytype
  wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);

  /// Test for conversions
  wasm_externtype_t *extfunc = wasm_functype_as_externtype(functype);
  EXPECT_EQ(functype, wasm_externtype_as_functype(extfunc));
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(extfunc));
  wasm_externtype_t *extglobal = wasm_globaltype_as_externtype(globaltype);
  EXPECT_EQ(globaltype, wasm_externtype_as_globaltype(extglobal));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(extglobal));
  wasm_externtype_t *exttable = wasm_tabletype_as_externtype(tabletype);
  EXPECT_EQ(tabletype, wasm_externtype_as_tabletype(exttable));
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(exttable));
  wasm_externtype_t *extmemory = wasm_memorytype_as_externtype(memorytype);
  EXPECT_EQ(memorytype, wasm_externtype_as_memorytype(extmemory));
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(extmemory));
  const wasm_externtype_t *cextfunc =
      wasm_functype_as_externtype_const(functype);
  EXPECT_EQ(functype, wasm_externtype_as_functype_const(cextfunc));
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(cextfunc));
  const wasm_externtype_t *cextglobal =
      wasm_globaltype_as_externtype_const(globaltype);
  EXPECT_EQ(globaltype, wasm_externtype_as_globaltype_const(cextglobal));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(cextglobal));
  const wasm_externtype_t *cexttable =
      wasm_tabletype_as_externtype_const(tabletype);
  EXPECT_EQ(tabletype, wasm_externtype_as_tabletype_const(cexttable));
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(cexttable));
  const wasm_externtype_t *cextmemory =
      wasm_memorytype_as_externtype_const(memorytype);
  EXPECT_EQ(memorytype, wasm_externtype_as_memorytype_const(cextmemory));
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(cextmemory));

  EXPECT_EQ(nullptr, wasm_externtype_as_functype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_functype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_globaltype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_globaltype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tabletype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tabletype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_memorytype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_memorytype_const(nullptr));
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(nullptr));

  wasm_externtype_t *externtypes[4] = {extfunc, extglobal, exttable, extmemory};
  wasm_externtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(externtype, 4, externtypes, out, copy)
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(
                          wasm_functype_params(
                              wasm_externtype_as_functype_const(copy.data[0]))
                              ->data[2]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(copy.data[1]))));
  EXPECT_EQ(WASM_ANYREF,
            wasm_valtype_kind(wasm_tabletype_element(
                wasm_externtype_as_tabletype_const(copy.data[2]))));
  EXPECT_EQ(10U, wasm_memorytype_limits(
                     wasm_externtype_as_memorytype_const(copy.data[3]))
                     ->min);
  EXPECT_EQ(20U, wasm_memorytype_limits(
                     wasm_externtype_as_memorytype_const(copy.data[3]))
                     ->max);
  WASM_DECLARE_VEC_DEL_TEST(externtype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ImportType) {
  /// Create globaltype
  wasm_globaltype_t *globaltypes[3] = {
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_CONST),
      wasm_globaltype_new(wasm_valtype_new(WASM_I64), WASM_VAR),
      wasm_globaltype_new(wasm_valtype_new(WASM_F32), WASM_CONST)};
  /// Create externtype
  wasm_externtype_t *externtypes[3] = {
      wasm_globaltype_as_externtype(globaltypes[0]),
      wasm_globaltype_as_externtype(globaltypes[1]),
      wasm_globaltype_as_externtype(globaltypes[2])};
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[0]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[1]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[2]));
  /// Create names
  wasm_name_t modnames[3], names[3];
  wasm_name_new_from_string(&modnames[0], "module");
  wasm_name_new_from_string(&modnames[1], "module");
  wasm_name_new_from_string(&modnames[2], "module");
  wasm_name_new_from_string(&names[0], "global1");
  wasm_name_new_from_string(&names[1], "global2");
  wasm_name_new_from_string(&names[2], "global3");
  wasm_importtype_t *importtypes[3] = {
      wasm_importtype_new(&modnames[0], &names[0], externtypes[0]),
      wasm_importtype_new(&modnames[1], &names[1], externtypes[1]),
      wasm_importtype_new(&modnames[2], &names[2], externtypes[2])};
  EXPECT_NE(nullptr, wasm_importtype_module(importtypes[0]));
  EXPECT_EQ(nullptr, wasm_importtype_module(nullptr));
  EXPECT_NE(nullptr, wasm_importtype_name(importtypes[0]));
  EXPECT_EQ(nullptr, wasm_importtype_name(nullptr));
  EXPECT_NE(nullptr, wasm_importtype_type(importtypes[0]));
  EXPECT_EQ(nullptr, wasm_importtype_type(nullptr));
  wasm_importtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(importtype, 3, importtypes, out, copy)
  EXPECT_EQ(std::string("module"),
            std::string(wasm_importtype_module(copy.data[0])->data,
                        wasm_importtype_module(copy.data[0])->size));
  EXPECT_EQ(std::string("module"),
            std::string(wasm_importtype_module(copy.data[1])->data,
                        wasm_importtype_module(copy.data[1])->size));
  EXPECT_EQ(std::string("module"),
            std::string(wasm_importtype_module(copy.data[2])->data,
                        wasm_importtype_module(copy.data[2])->size));
  EXPECT_EQ(std::string("global1"),
            std::string(wasm_importtype_name(copy.data[0])->data,
                        wasm_importtype_name(copy.data[0])->size));
  EXPECT_EQ(std::string("global2"),
            std::string(wasm_importtype_name(copy.data[1])->data,
                        wasm_importtype_name(copy.data[1])->size));
  EXPECT_EQ(std::string("global3"),
            std::string(wasm_importtype_name(copy.data[2])->data,
                        wasm_importtype_name(copy.data[2])->size));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_importtype_type(copy.data[0])))));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_importtype_type(copy.data[1])))));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_importtype_type(copy.data[2])))));
  WASM_DECLARE_VEC_DEL_TEST(importtype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ExportType) {
  /// Create globaltype
  wasm_globaltype_t *globaltypes[3] = {
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_CONST),
      wasm_globaltype_new(wasm_valtype_new(WASM_I64), WASM_VAR),
      wasm_globaltype_new(wasm_valtype_new(WASM_F32), WASM_CONST)};
  /// Create externtype
  wasm_externtype_t *externtypes[3] = {
      wasm_globaltype_as_externtype(globaltypes[0]),
      wasm_globaltype_as_externtype(globaltypes[1]),
      wasm_globaltype_as_externtype(globaltypes[2])};
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[0]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[1]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[2]));
  /// Create names
  wasm_name_t names[3];
  wasm_name_new_from_string(&names[0], "global1");
  wasm_name_new_from_string(&names[1], "global2");
  wasm_name_new_from_string(&names[2], "global3");
  wasm_exporttype_t *exporttypes[3] = {
      wasm_exporttype_new(&names[0], externtypes[0]),
      wasm_exporttype_new(&names[1], externtypes[1]),
      wasm_exporttype_new(&names[2], externtypes[2])};
  EXPECT_NE(nullptr, wasm_exporttype_name(exporttypes[0]));
  EXPECT_EQ(nullptr, wasm_exporttype_name(nullptr));
  EXPECT_NE(nullptr, wasm_exporttype_type(exporttypes[0]));
  EXPECT_EQ(nullptr, wasm_exporttype_type(nullptr));
  wasm_exporttype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(exporttype, 3, exporttypes, out, copy)
  EXPECT_EQ(std::string("global1"),
            std::string(wasm_exporttype_name(copy.data[0])->data,
                        wasm_exporttype_name(copy.data[0])->size));
  EXPECT_EQ(std::string("global2"),
            std::string(wasm_exporttype_name(copy.data[1])->data,
                        wasm_exporttype_name(copy.data[1])->size));
  EXPECT_EQ(std::string("global3"),
            std::string(wasm_exporttype_name(copy.data[2])->data,
                        wasm_exporttype_name(copy.data[2])->size));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_exporttype_type(copy.data[0])))));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_exporttype_type(copy.data[1])))));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(
                              wasm_exporttype_type(copy.data[2])))));
  WASM_DECLARE_VEC_DEL_TEST(exporttype, out, copy)
  EXPECT_TRUE(true);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
