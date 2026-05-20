// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wasm.h"

#include "gtest/gtest.h"

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

// Helper macro for the WASM_DECLARE_OWN_IMPL test.
#define WASM_DECLARE_OWN_TEST(name, obj)                                       \
  wasm_##name##_delete(nullptr);                                               \
  wasm_##name##_delete(obj);

// Helper macro for the WASM_DECLARE_VEC_IMPL test.
#define WASM_DECLARE_VEC_TEST(name, n, vals, out, copy)                        \
  wasm_##name##_vec_new_empty(nullptr);                                        \
  wasm_##name##_vec_new_empty(&out);                                           \
  EXPECT_EQ(0U, out.size);                                                     \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new_uninitialized(nullptr, n);                             \
  wasm_##name##_vec_new_uninitialized(&out, 0);                                \
  EXPECT_EQ(0U, out.size);                                                     \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new_uninitialized(&out, n);                                \
  EXPECT_EQ((size_t)(n), out.size);                                            \
  EXPECT_NE(nullptr, out.data);                                                \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new(nullptr, n, vals);                                     \
  wasm_##name##_vec_new(&out, 0, nullptr);                                     \
  EXPECT_EQ(0U, out.size);                                                     \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_new(&out, n, vals);                                        \
  EXPECT_EQ((size_t)(n), out.size);                                            \
  EXPECT_NE(nullptr, out.data);                                                \
  wasm_##name##_vec_new_empty(&copy);                                          \
  wasm_##name##_vec_copy(nullptr, nullptr);                                    \
  wasm_##name##_vec_copy(nullptr, &out);                                       \
  wasm_##name##_vec_copy(&copy, nullptr);                                      \
  wasm_##name##_vec_copy(&copy, &out);                                         \
  EXPECT_EQ(out.size, copy.size);

// Helper macro for the deletion function of WASM_DECLARE_VEC_IMPL test.
#define WASM_DECLARE_VEC_DEL_TEST(name, out, copy)                             \
  wasm_##name##_vec_delete(&out);                                              \
  wasm_##name##_vec_delete(&copy);                                             \
  wasm_##name##_vec_delete(&out);

// Helper macro for the WASM_DECLARE_TYPE_IMPL test.
#define WASM_DECLARE_TYPE_TEST(name, n, vals, out, copy)                       \
  wasm_##name##_t *dup_##name = wasm_##name##_copy(vals[0]);                   \
  EXPECT_NE(nullptr, dup_##name);                                              \
  WASM_DECLARE_OWN_TEST(name, dup_##name)                                      \
  dup_##name = wasm_##name##_copy(nullptr);                                    \
  EXPECT_EQ(nullptr, dup_##name);                                              \
  WASM_DECLARE_VEC_TEST(name, n, vals, out, copy)

// Recording finalizer for WASM_DECLARE_REF_BASE_TEST.
extern int FinalizerCalled;
extern void recordingFinalizer(void *env);

// Helper macro for the WASM_DECLARE_REF_BASE_IMPL test.
#define WASM_DECLARE_REF_BASE_TEST(name, obj, fin_obj)                         \
  /* copy + same */                                                            \
  {                                                                            \
    wasm_##name##_t *cp_##name = wasm_##name##_copy(obj);                      \
    ASSERT_NE(nullptr, cp_##name);                                             \
    EXPECT_TRUE(wasm_##name##_same(obj, cp_##name));                           \
    wasm_##name##_delete(cp_##name);                                           \
  }                                                                            \
  /* host_info round-trip */                                                   \
  {                                                                            \
    int marker_##name = 0;                                                     \
    EXPECT_EQ(nullptr, wasm_##name##_get_host_info(obj));                      \
    wasm_##name##_set_host_info(obj, &marker_##name);                          \
    EXPECT_EQ(&marker_##name, wasm_##name##_get_host_info(obj));               \
    wasm_##name##_set_host_info(obj, nullptr);                                 \
  }                                                                            \
  /* host_info_with_finalizer: fires on the last delete */                     \
  {                                                                            \
    FinalizerCalled = 0;                                                       \
    int env_##name = 0;                                                        \
    wasm_##name##_set_host_info_with_finalizer(fin_obj, &env_##name,           \
                                               recordingFinalizer);            \
    EXPECT_EQ(&env_##name, wasm_##name##_get_host_info(fin_obj));              \
    wasm_##name##_delete(fin_obj);                                             \
    EXPECT_EQ(1, FinalizerCalled);                                             \
    EXPECT_EQ(1, env_##name);                                                  \
  }

// Helper macro for the WASM_DECLARE_REF_IMPL test.
#define WASM_DECLARE_REF_TEST(name, obj, fin_obj)                              \
  WASM_DECLARE_REF_BASE_TEST(name, obj, fin_obj) {                             \
    wasm_ref_t *r_##name = wasm_##name##_as_ref(obj);                          \
    ASSERT_NE(nullptr, r_##name);                                              \
    EXPECT_EQ(obj, wasm_ref_as_##name(r_##name));                              \
    const wasm_ref_t *rc_##name = wasm_##name##_as_ref_const(obj);             \
    EXPECT_EQ(obj, wasm_ref_as_##name##_const(rc_##name));                     \
    EXPECT_EQ(nullptr, wasm_##name##_as_ref(nullptr));                         \
    EXPECT_EQ(nullptr, wasm_ref_as_##name(nullptr));                           \
    EXPECT_EQ(nullptr, wasm_##name##_as_ref_const(nullptr));                   \
    EXPECT_EQ(nullptr, wasm_ref_as_##name##_const(nullptr));                   \
  }

// Helper macro for the WASM_DECLARE_SHARABLE_REF_IMPL test.
#define WASM_DECLARE_SHARABLE_REF_TEST(name, obj, fin_obj, store)              \
  WASM_DECLARE_REF_TEST(name, obj, fin_obj) {                                  \
    wasm_shared_##name##_t *sh_##name = wasm_##name##_share(obj);              \
    ASSERT_NE(nullptr, sh_##name);                                             \
    wasm_##name##_t *obtained_##name = wasm_##name##_obtain(store, sh_##name); \
    ASSERT_NE(nullptr, obtained_##name);                                       \
    wasm_##name##_delete(obtained_##name);                                     \
    EXPECT_EQ(nullptr, wasm_##name##_share(nullptr));                          \
    EXPECT_EQ(nullptr, wasm_##name##_obtain(nullptr, sh_##name));              \
    EXPECT_EQ(nullptr, wasm_##name##_obtain(store, nullptr));                  \
    WASM_DECLARE_OWN_TEST(shared_##name, sh_##name)                            \
  }

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
      wasm_valtype_new(WASM_I32),       wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32),       wasm_valtype_new(WASM_F64),
      wasm_valtype_new(WASM_EXTERNREF), wasm_valtype_new(WASM_FUNCREF)};
  EXPECT_EQ(WASM_EXTERNREF, wasm_valtype_kind(valtypes[4]));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(nullptr));
  wasm_valtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(valtype, 6, valtypes, out, copy)
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(copy.data[0]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(copy.data[1]));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(copy.data[2]));
  EXPECT_EQ(WASM_F64, wasm_valtype_kind(copy.data[3]));
  EXPECT_EQ(WASM_EXTERNREF, wasm_valtype_kind(copy.data[4]));
  EXPECT_EQ(WASM_FUNCREF, wasm_valtype_kind(copy.data[5]));
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(out.data[0]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(out.data[1]));
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(out.data[2]));
  EXPECT_EQ(WASM_F64, wasm_valtype_kind(out.data[3]));
  EXPECT_EQ(WASM_EXTERNREF, wasm_valtype_kind(out.data[4]));
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
      wasm_valtype_new(WASM_I32),       wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32),       wasm_valtype_new(WASM_F64),
      wasm_valtype_new(WASM_EXTERNREF), wasm_valtype_new(WASM_FUNCREF)};
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
  EXPECT_EQ(WASM_EXTERNREF,
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
  wasm_valtype_t *valtypes[3] = {wasm_valtype_new(WASM_EXTERNREF),
                                 wasm_valtype_new(WASM_FUNCREF),
                                 wasm_valtype_new(WASM_FUNCREF)};
  wasm_limits_t limits = {10, 20};
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
  EXPECT_EQ(WASM_EXTERNREF,
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
  wasm_limits_t limits = {10, 20};
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

TEST(APIWasmCTest, TagType) {
  auto makeTagFunctype = [](wasm_valkind_t kind) {
    wasm_valtype_t *vs[1] = {wasm_valtype_new(kind)};
    wasm_valtype_vec_t params, results;
    wasm_valtype_vec_new(&params, 1, vs);
    wasm_valtype_vec_new_empty(&results);
    return wasm_functype_new(&params, &results);
  };
  wasm_tagtype_t *tagtypes[3] = {wasm_tagtype_new(makeTagFunctype(WASM_I32)),
                                 wasm_tagtype_new(makeTagFunctype(WASM_I64)),
                                 wasm_tagtype_new(makeTagFunctype(WASM_F32))};
  ASSERT_NE(nullptr, tagtypes[0]);
  EXPECT_NE(nullptr, wasm_tagtype_functype(tagtypes[0]));
  EXPECT_EQ(nullptr, wasm_tagtype_functype(nullptr));
  EXPECT_EQ(nullptr, wasm_tagtype_new(nullptr));

  wasm_tagtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(tagtype, 3, tagtypes, out, copy)

  // Verify each tagtype carries its original parameter kind through the
  // vec_copy round-trip.
  EXPECT_EQ(
      WASM_I32,
      wasm_valtype_kind(
          wasm_functype_params(wasm_tagtype_functype(copy.data[0]))->data[0]));
  EXPECT_EQ(
      WASM_I64,
      wasm_valtype_kind(
          wasm_functype_params(wasm_tagtype_functype(copy.data[1]))->data[0]));
  EXPECT_EQ(
      WASM_F32,
      wasm_valtype_kind(
          wasm_functype_params(wasm_tagtype_functype(copy.data[2]))->data[0]));
  WASM_DECLARE_VEC_DEL_TEST(tagtype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ExternType) {
  // Create functype
  wasm_valtype_vec_t params, results;
  wasm_valtype_t *valtypes[4] = {
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I64),
      wasm_valtype_new(WASM_F32), wasm_valtype_new(WASM_F64)};
  wasm_valtype_vec_new(&params, 4, valtypes);
  wasm_valtype_vec_copy(&results, &params);
  wasm_functype_t *functype = wasm_functype_new(&params, &results);
  // Create globaltype
  wasm_valtype_t *valtype = wasm_valtype_new(WASM_I64);
  wasm_globaltype_t *globaltype = wasm_globaltype_new(valtype, WASM_VAR);
  // Create tabletype
  wasm_limits_t limits = {10, 20};
  valtype = wasm_valtype_new(WASM_EXTERNREF);
  wasm_tabletype_t *tabletype = wasm_tabletype_new(valtype, &limits);
  // Create memorytype
  wasm_memorytype_t *memorytype = wasm_memorytype_new(&limits);
  // Create tagtype (i32 -> ())
  wasm_valtype_t *tagvs[1] = {wasm_valtype_new(WASM_I32)};
  wasm_valtype_vec_t tagparams, tagresults;
  wasm_valtype_vec_new(&tagparams, 1, tagvs);
  wasm_valtype_vec_new_empty(&tagresults);
  wasm_tagtype_t *tagtype =
      wasm_tagtype_new(wasm_functype_new(&tagparams, &tagresults));

  // Test for conversions
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
  wasm_externtype_t *exttag = wasm_tagtype_as_externtype(tagtype);
  EXPECT_EQ(tagtype, wasm_externtype_as_tagtype(exttag));
  EXPECT_EQ(WASM_EXTERN_TAG, wasm_externtype_kind(exttag));
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
  const wasm_externtype_t *cexttag = wasm_tagtype_as_externtype_const(tagtype);
  EXPECT_EQ(tagtype, wasm_externtype_as_tagtype_const(cexttag));
  EXPECT_EQ(WASM_EXTERN_TAG, wasm_externtype_kind(cexttag));

  EXPECT_EQ(nullptr, wasm_externtype_as_functype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_functype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_globaltype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_globaltype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tabletype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tabletype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_memorytype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_memorytype_const(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tagtype(nullptr));
  EXPECT_EQ(nullptr, wasm_externtype_as_tagtype_const(nullptr));
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(nullptr));

  wasm_externtype_t *externtypes[5] = {extfunc, extglobal, exttable, extmemory,
                                       exttag};
  wasm_externtype_vec_t out, copy;
  WASM_DECLARE_TYPE_TEST(externtype, 5, externtypes, out, copy)
  EXPECT_EQ(WASM_F32, wasm_valtype_kind(
                          wasm_functype_params(
                              wasm_externtype_as_functype_const(copy.data[0]))
                              ->data[2]));
  EXPECT_EQ(WASM_I64, wasm_valtype_kind(wasm_globaltype_content(
                          wasm_externtype_as_globaltype_const(copy.data[1]))));
  EXPECT_EQ(WASM_EXTERNREF,
            wasm_valtype_kind(wasm_tabletype_element(
                wasm_externtype_as_tabletype_const(copy.data[2]))));
  EXPECT_EQ(10U, wasm_memorytype_limits(
                     wasm_externtype_as_memorytype_const(copy.data[3]))
                     ->min);
  EXPECT_EQ(20U, wasm_memorytype_limits(
                     wasm_externtype_as_memorytype_const(copy.data[3]))
                     ->max);
  EXPECT_EQ(
      WASM_I32,
      wasm_valtype_kind(wasm_functype_params(
                            wasm_tagtype_functype(
                                wasm_externtype_as_tagtype_const(copy.data[4])))
                            ->data[0]));
  WASM_DECLARE_VEC_DEL_TEST(externtype, out, copy)
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, ImportType) {
  // Create globaltype
  wasm_globaltype_t *globaltypes[3] = {
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_CONST),
      wasm_globaltype_new(wasm_valtype_new(WASM_I64), WASM_VAR),
      wasm_globaltype_new(wasm_valtype_new(WASM_F32), WASM_CONST)};
  // Create externtype
  wasm_externtype_t *externtypes[3] = {
      wasm_globaltype_as_externtype(globaltypes[0]),
      wasm_globaltype_as_externtype(globaltypes[1]),
      wasm_globaltype_as_externtype(globaltypes[2])};
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[0]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[1]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[2]));
  // Create names
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
  // Create globaltype
  wasm_globaltype_t *globaltypes[3] = {
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_CONST),
      wasm_globaltype_new(wasm_valtype_new(WASM_I64), WASM_VAR),
      wasm_globaltype_new(wasm_valtype_new(WASM_F32), WASM_CONST)};
  // Create externtype
  wasm_externtype_t *externtypes[3] = {
      wasm_globaltype_as_externtype(globaltypes[0]),
      wasm_globaltype_as_externtype(globaltypes[1]),
      wasm_globaltype_as_externtype(globaltypes[2])};
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[0]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[1]));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(externtypes[2]));
  // Create names
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

// Minimal valid WebAssembly module binary: (module) with no contents.
const wasm_byte_t MinimalModuleBin[8] = {0x00, 0x61, 0x73, 0x6d,
                                         0x01, 0x00, 0x00, 0x00};

// A trivial host function callback used by Func / Extern tests.
wasm_trap_t *trivialCallback(const wasm_val_vec_t *, wasm_val_vec_t *) {
  return nullptr;
}

int FinalizerCalled = 0;
void recordingFinalizer(void *env) {
  FinalizerCalled++;
  if (env) {
    *static_cast<int *>(env) = 1;
  }
}

TEST(APIWasmCTest, Frame) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // Mint a frame via a trap built with wasm_trap_new. Per impl, even
  // user-built traps carry an origin frame (funcidx 0).
  wasm_byte_vec_t msg;
  wasm_name_new_from_string_nt(&msg, "trap-for-frame");
  wasm_trap_t *trap = wasm_trap_new(store, &msg);
  wasm_byte_vec_delete(&msg);
  ASSERT_NE(nullptr, trap);

  wasm_frame_t *frame = wasm_trap_origin(trap);
  ASSERT_NE(nullptr, frame);

  // Accessors.
  EXPECT_EQ(0U, wasm_frame_func_index(frame));
  EXPECT_EQ(0U, wasm_frame_func_offset(frame));
  EXPECT_EQ(0U, wasm_frame_module_offset(frame));
  // The instance pointer is intentionally null in our impl.
  EXPECT_EQ(nullptr, wasm_frame_instance(frame));

  // Null guards.
  EXPECT_EQ(0U, wasm_frame_func_index(nullptr));
  EXPECT_EQ(0U, wasm_frame_func_offset(nullptr));
  EXPECT_EQ(0U, wasm_frame_module_offset(nullptr));
  EXPECT_EQ(nullptr, wasm_frame_instance(nullptr));

  // Copy.
  wasm_frame_t *frame_copy = wasm_frame_copy(frame);
  ASSERT_NE(nullptr, frame_copy);
  EXPECT_EQ(0U, wasm_frame_func_index(frame_copy));

  // wasm_frame_delete null guard via WASM_DECLARE_OWN_TEST.
  WASM_DECLARE_OWN_TEST(frame, frame_copy)

  // Frame vec ops. Build several frames from the same trap so the vec test
  // has real values to work with.
  wasm_frame_t *frames[3] = {wasm_trap_origin(trap), wasm_trap_origin(trap),
                             wasm_trap_origin(trap)};
  wasm_frame_vec_t out, copy;
  WASM_DECLARE_VEC_TEST(frame, 3, frames, out, copy)
  WASM_DECLARE_VEC_DEL_TEST(frame, out, copy)

  wasm_frame_delete(frame);
  wasm_trap_delete(trap);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Trap) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // Null guards on wasm_trap_new.
  wasm_byte_vec_t msg;
  wasm_name_new_from_string_nt(&msg, "boom");
  EXPECT_EQ(nullptr, wasm_trap_new(nullptr, &msg));
  EXPECT_EQ(nullptr, wasm_trap_new(store, nullptr));

  wasm_trap_t *trap = wasm_trap_new(store, &msg);
  wasm_byte_vec_delete(&msg);
  ASSERT_NE(nullptr, trap);

  // Independent finalizer-target trap needs its own message.
  wasm_byte_vec_t fin_msg;
  wasm_name_new_from_string_nt(&fin_msg, "boom-fin");
  wasm_trap_t *trap_fin = wasm_trap_new(store, &fin_msg);
  wasm_byte_vec_delete(&fin_msg);
  ASSERT_NE(nullptr, trap_fin);

  // wasm_trap_message round-trip.
  wasm_message_t out_msg;
  wasm_trap_message(trap, &out_msg);
  ASSERT_NE(nullptr, out_msg.data);
  EXPECT_EQ(std::string("boom"), std::string(out_msg.data, out_msg.size - 1));
  wasm_byte_vec_delete(&out_msg);
  // Null guards on message.
  wasm_trap_message(nullptr, &out_msg);
  EXPECT_EQ(0U, out_msg.size);
  EXPECT_EQ(nullptr, out_msg.data);
  wasm_trap_message(trap, nullptr);

  // wasm_trap_origin returns a non-null frame.
  wasm_frame_t *origin = wasm_trap_origin(trap);
  ASSERT_NE(nullptr, origin);
  EXPECT_EQ(0U, wasm_frame_func_index(origin));
  wasm_frame_delete(origin);
  EXPECT_EQ(nullptr, wasm_trap_origin(nullptr));

  // wasm_trap_trace yields a 1-element vec for user-built traps.
  wasm_frame_vec_t trace;
  wasm_trap_trace(trap, &trace);
  EXPECT_EQ(1U, trace.size);
  wasm_frame_vec_delete(&trace);
  // Null guards on trace.
  wasm_trap_trace(nullptr, &trace);
  EXPECT_EQ(0U, trace.size);
  EXPECT_EQ(nullptr, trace.data);
  wasm_trap_trace(trap, nullptr);

  WASM_DECLARE_REF_TEST(trap, trap, trap_fin)

  // wasm-c-api does not define wasm_trap_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(trap, trap)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Foreign) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // wasm_foreign_new with null store returns null.
  EXPECT_EQ(nullptr, wasm_foreign_new(nullptr));

  wasm_foreign_t *foreign = wasm_foreign_new(store);
  ASSERT_NE(nullptr, foreign);
  wasm_foreign_t *foreign_fin = wasm_foreign_new(store);
  ASSERT_NE(nullptr, foreign_fin);

  WASM_DECLARE_REF_TEST(foreign, foreign, foreign_fin)

  // wasm-c-api does not define wasm_foreign_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(foreign, foreign)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Module) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t bin;
  wasm_byte_vec_new(&bin, sizeof(MinimalModuleBin), MinimalModuleBin);

  // wasm_module_validate null guards + happy path.
  EXPECT_FALSE(wasm_module_validate(nullptr, &bin));
  EXPECT_FALSE(wasm_module_validate(store, nullptr));
  EXPECT_TRUE(wasm_module_validate(store, &bin));

  // wasm_module_new null guards + happy path.
  EXPECT_EQ(nullptr, wasm_module_new(nullptr, &bin));
  EXPECT_EQ(nullptr, wasm_module_new(store, nullptr));
  wasm_module_t *module = wasm_module_new(store, &bin);
  wasm_module_t *module_fin = wasm_module_new(store, &bin);
  wasm_byte_vec_delete(&bin);
  ASSERT_NE(nullptr, module);
  ASSERT_NE(nullptr, module_fin);

  // imports / exports both yield empty vecs for (module).
  wasm_importtype_vec_t imports;
  wasm_module_imports(module, &imports);
  EXPECT_EQ(0U, imports.size);
  wasm_importtype_vec_delete(&imports);
  // Null guards on imports.
  wasm_module_imports(nullptr, &imports);
  EXPECT_EQ(0U, imports.size);
  EXPECT_EQ(nullptr, imports.data);
  wasm_module_imports(module, nullptr);

  wasm_exporttype_vec_t exports;
  wasm_module_exports(module, &exports);
  EXPECT_EQ(0U, exports.size);
  wasm_exporttype_vec_delete(&exports);
  wasm_module_exports(nullptr, &exports);
  EXPECT_EQ(0U, exports.size);
  EXPECT_EQ(nullptr, exports.data);
  wasm_module_exports(module, nullptr);

  // serialize + deserialize round-trip.
  wasm_byte_vec_t serialized;
  wasm_module_serialize(module, &serialized);
  ASSERT_NE(nullptr, serialized.data);
  ASSERT_LT(0U, serialized.size);
  wasm_module_serialize(nullptr, &serialized);
  EXPECT_EQ(0U, serialized.size);
  EXPECT_EQ(nullptr, serialized.data);
  wasm_module_serialize(module, nullptr);

  wasm_module_serialize(module, &serialized);
  wasm_module_t *module2 = wasm_module_deserialize(store, &serialized);
  wasm_byte_vec_delete(&serialized);
  ASSERT_NE(nullptr, module2);
  wasm_module_delete(module2);

  WASM_DECLARE_SHARABLE_REF_TEST(module, module, module_fin, store)

  // wasm-c-api does not define wasm_module_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(module, module)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Func) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_functype_t *ftype = wasm_functype_new_0_0();

  // wasm_func_new null guards.
  EXPECT_EQ(nullptr, wasm_func_new(nullptr, ftype, trivialCallback));
  EXPECT_EQ(nullptr, wasm_func_new(store, nullptr, trivialCallback));
  EXPECT_EQ(nullptr, wasm_func_new(store, ftype, nullptr));

  wasm_func_t *func = wasm_func_new(store, ftype, trivialCallback);
  ASSERT_NE(nullptr, func);
  wasm_func_t *func_fin = wasm_func_new(store, ftype, trivialCallback);
  ASSERT_NE(nullptr, func_fin);

  // type / param_arity / result_arity.
  wasm_functype_t *got_type = wasm_func_type(func);
  ASSERT_NE(nullptr, got_type);
  EXPECT_EQ(0U, wasm_functype_params(got_type)->size);
  EXPECT_EQ(0U, wasm_functype_results(got_type)->size);
  wasm_functype_delete(got_type);
  EXPECT_EQ(nullptr, wasm_func_type(nullptr));
  EXPECT_EQ(0U, wasm_func_param_arity(func));
  EXPECT_EQ(0U, wasm_func_result_arity(func));
  EXPECT_EQ(0U, wasm_func_param_arity(nullptr));
  EXPECT_EQ(0U, wasm_func_result_arity(nullptr));

  // as_extern / extern_as_func round-trip.
  wasm_extern_t *as_ext = wasm_func_as_extern(func);
  ASSERT_NE(nullptr, as_ext);
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_extern_kind(as_ext));
  EXPECT_EQ(func, wasm_extern_as_func(as_ext));
  EXPECT_EQ(nullptr, wasm_func_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_func(nullptr));
  const wasm_extern_t *as_ext_c = wasm_func_as_extern_const(func);
  EXPECT_EQ(func, wasm_extern_as_func_const(as_ext_c));

  WASM_DECLARE_REF_TEST(func, func, func_fin)

  // wasm_func_new_with_env happy path + env + finalizer.
  FinalizerCalled = 0;
  int env_slot = 0;
  EXPECT_EQ(nullptr, wasm_func_new_with_env(nullptr, ftype, nullptr, &env_slot,
                                            recordingFinalizer));
  wasm_func_t *envfunc = wasm_func_new_with_env(
      store, ftype,
      [](void *, const wasm_val_vec_t *, wasm_val_vec_t *) -> wasm_trap_t * {
        return nullptr;
      },
      &env_slot, recordingFinalizer);
  ASSERT_NE(nullptr, envfunc);
  wasm_func_delete(envfunc);
  EXPECT_EQ(1, FinalizerCalled);
  EXPECT_EQ(1, env_slot);

  wasm_functype_delete(ftype);

  // wasm-c-api does not define wasm_func_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(func, func)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Global) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_globaltype_t *gtype =
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_VAR);
  wasm_val_t init = i32Val(11);

  // wasm_global_new null guards.
  EXPECT_EQ(nullptr, wasm_global_new(nullptr, gtype, &init));
  EXPECT_EQ(nullptr, wasm_global_new(store, nullptr, &init));
  EXPECT_EQ(nullptr, wasm_global_new(store, gtype, nullptr));

  wasm_global_t *global = wasm_global_new(store, gtype, &init);
  ASSERT_NE(nullptr, global);
  wasm_global_t *global_fin = wasm_global_new(store, gtype, &init);
  ASSERT_NE(nullptr, global_fin);

  // type.
  wasm_globaltype_t *got_type = wasm_global_type(global);
  ASSERT_NE(nullptr, got_type);
  EXPECT_EQ(WASM_I32, wasm_valtype_kind(wasm_globaltype_content(got_type)));
  EXPECT_EQ(WASM_VAR, wasm_globaltype_mutability(got_type));
  wasm_globaltype_delete(got_type);
  EXPECT_EQ(nullptr, wasm_global_type(nullptr));

  // get / set round-trip.
  wasm_val_t got{};
  wasm_global_get(global, &got);
  EXPECT_EQ(WASM_I32, got.kind);
  EXPECT_EQ(11, got.of.i32);
  wasm_val_t newv = i32Val(99);
  wasm_global_set(global, &newv);
  wasm_global_get(global, &got);
  EXPECT_EQ(99, got.of.i32);
  // Null guards on get/set.
  wasm_global_get(nullptr, &got);
  wasm_global_get(global, nullptr);
  wasm_global_set(nullptr, &newv);
  wasm_global_set(global, nullptr);

  // as_extern.
  wasm_extern_t *as_ext = wasm_global_as_extern(global);
  ASSERT_NE(nullptr, as_ext);
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_extern_kind(as_ext));
  EXPECT_EQ(global, wasm_extern_as_global(as_ext));
  EXPECT_EQ(nullptr, wasm_global_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_global(nullptr));
  const wasm_extern_t *as_ext_c = wasm_global_as_extern_const(global);
  EXPECT_EQ(global, wasm_extern_as_global_const(as_ext_c));

  WASM_DECLARE_REF_TEST(global, global, global_fin)

  wasm_globaltype_delete(gtype);

  // wasm-c-api does not define wasm_global_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(global, global)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Table) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_limits_t limits = {1, 5};
  wasm_tabletype_t *ttype =
      wasm_tabletype_new(wasm_valtype_new(WASM_EXTERNREF), &limits);
  wasm_table_t *table = wasm_table_new(store, ttype, nullptr);
  ASSERT_NE(nullptr, table);
  wasm_table_t *table_fin = wasm_table_new(store, ttype, nullptr);
  ASSERT_NE(nullptr, table_fin);

  // type.
  wasm_tabletype_t *got_type = wasm_table_type(table);
  ASSERT_NE(nullptr, got_type);
  EXPECT_EQ(WASM_EXTERNREF,
            wasm_valtype_kind(wasm_tabletype_element(got_type)));
  wasm_tabletype_delete(got_type);
  EXPECT_EQ(nullptr, wasm_table_type(nullptr));

  // size.
  EXPECT_EQ(1U, wasm_table_size(table));
  EXPECT_EQ(0U, wasm_table_size(nullptr));

  // get / set with a foreign ref.
  EXPECT_EQ(nullptr, wasm_table_get(table, 0));
  EXPECT_EQ(nullptr, wasm_table_get(nullptr, 0));
  wasm_foreign_t *foreign = wasm_foreign_new(store);
  ASSERT_NE(nullptr, foreign);
  wasm_ref_t *foreign_ref = wasm_foreign_as_ref(foreign);
  EXPECT_TRUE(wasm_table_set(table, 0, foreign_ref));
  EXPECT_FALSE(wasm_table_set(nullptr, 0, foreign_ref));
  EXPECT_FALSE(wasm_table_set(table, 100, foreign_ref));
  wasm_ref_t *got_ref = wasm_table_get(table, 0);
  EXPECT_NE(nullptr, got_ref);
  wasm_ref_delete(got_ref);

  // grow.
  EXPECT_TRUE(wasm_table_grow(table, 1, nullptr));
  EXPECT_EQ(2U, wasm_table_size(table));
  EXPECT_FALSE(wasm_table_grow(nullptr, 1, nullptr));

  // as_extern.
  wasm_extern_t *as_ext = wasm_table_as_extern(table);
  ASSERT_NE(nullptr, as_ext);
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_extern_kind(as_ext));
  EXPECT_EQ(table, wasm_extern_as_table(as_ext));
  EXPECT_EQ(nullptr, wasm_table_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_table(nullptr));
  const wasm_extern_t *as_ext_c = wasm_table_as_extern_const(table);
  EXPECT_EQ(table, wasm_extern_as_table_const(as_ext_c));

  WASM_DECLARE_REF_TEST(table, table, table_fin)

  wasm_tabletype_delete(ttype);
  wasm_foreign_delete(foreign);

  // wasm-c-api does not define wasm_table_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(table, table)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Memory) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_limits_t limits = {1, 2};
  wasm_memorytype_t *mtype = wasm_memorytype_new(&limits);
  wasm_memory_t *memory = wasm_memory_new(store, mtype);
  ASSERT_NE(nullptr, memory);
  wasm_memory_t *memory_fin = wasm_memory_new(store, mtype);
  ASSERT_NE(nullptr, memory_fin);

  // type.
  wasm_memorytype_t *got_type = wasm_memory_type(memory);
  ASSERT_NE(nullptr, got_type);
  EXPECT_EQ(1U, wasm_memorytype_limits(got_type)->min);
  EXPECT_EQ(2U, wasm_memorytype_limits(got_type)->max);
  wasm_memorytype_delete(got_type);
  EXPECT_EQ(nullptr, wasm_memory_type(nullptr));

  // data / size / data_size.
  EXPECT_NE(nullptr, wasm_memory_data(memory));
  EXPECT_EQ(nullptr, wasm_memory_data(nullptr));
  EXPECT_EQ(1U, wasm_memory_size(memory));
  EXPECT_EQ(0U, wasm_memory_size(nullptr));
  EXPECT_EQ(0x10000U, wasm_memory_data_size(memory));
  EXPECT_EQ(0U, wasm_memory_data_size(nullptr));

  // grow: 1 -> 2 pages succeeds, 2 -> 3 (over max) fails.
  EXPECT_TRUE(wasm_memory_grow(memory, 1));
  EXPECT_EQ(2U, wasm_memory_size(memory));
  EXPECT_EQ(0x20000U, wasm_memory_data_size(memory));
  EXPECT_FALSE(wasm_memory_grow(memory, 1));
  EXPECT_FALSE(wasm_memory_grow(nullptr, 1));

  // as_extern.
  wasm_extern_t *as_ext = wasm_memory_as_extern(memory);
  ASSERT_NE(nullptr, as_ext);
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_extern_kind(as_ext));
  EXPECT_EQ(memory, wasm_extern_as_memory(as_ext));
  EXPECT_EQ(nullptr, wasm_memory_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_memory(nullptr));
  const wasm_extern_t *as_ext_c = wasm_memory_as_extern_const(memory);
  EXPECT_EQ(memory, wasm_extern_as_memory_const(as_ext_c));

  WASM_DECLARE_REF_TEST(memory, memory, memory_fin)

  wasm_memorytype_delete(mtype);

  // wasm-c-api does not define wasm_memory_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(memory, memory)
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Instance) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  wasm_byte_vec_t bin;
  wasm_byte_vec_new(&bin, sizeof(MinimalModuleBin), MinimalModuleBin);
  wasm_module_t *module = wasm_module_new(store, &bin);
  wasm_byte_vec_delete(&bin);
  ASSERT_NE(nullptr, module);

  wasm_extern_vec_t empty_imports = WASM_EMPTY_VEC;

  // wasm_instance_new null guards.
  EXPECT_EQ(nullptr,
            wasm_instance_new(nullptr, module, &empty_imports, nullptr));
  EXPECT_EQ(nullptr,
            wasm_instance_new(store, nullptr, &empty_imports, nullptr));

  wasm_instance_t *instance =
      wasm_instance_new(store, module, &empty_imports, nullptr);
  ASSERT_NE(nullptr, instance);
  wasm_instance_t *instance_fin =
      wasm_instance_new(store, module, &empty_imports, nullptr);
  ASSERT_NE(nullptr, instance_fin);

  // exports yields an empty vec for (module).
  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  EXPECT_EQ(0U, exports.size);
  wasm_extern_vec_delete(&exports);

  WASM_DECLARE_REF_TEST(instance, instance, instance_fin)

  // wasm-c-api does not define wasm_instance_vec_t, so no vec ops here.

  WASM_DECLARE_OWN_TEST(instance, instance)
  wasm_module_delete(module);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Extern) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // Build one of each extern kind.
  wasm_functype_t *ftype = wasm_functype_new_0_0();
  wasm_func_t *f = wasm_func_new(store, ftype, trivialCallback);
  wasm_func_t *f_fin = wasm_func_new(store, ftype, trivialCallback);
  wasm_functype_delete(ftype);

  wasm_globaltype_t *gtype =
      wasm_globaltype_new(wasm_valtype_new(WASM_I32), WASM_VAR);
  wasm_val_t initv = i32Val(0);
  wasm_global_t *g = wasm_global_new(store, gtype, &initv);
  wasm_globaltype_delete(gtype);

  wasm_limits_t tlimits = {1, 5};
  wasm_tabletype_t *ttype =
      wasm_tabletype_new(wasm_valtype_new(WASM_EXTERNREF), &tlimits);
  wasm_table_t *t = wasm_table_new(store, ttype, nullptr);
  wasm_tabletype_delete(ttype);

  wasm_limits_t mlimits = {1, 2};
  wasm_memorytype_t *mtype = wasm_memorytype_new(&mlimits);
  wasm_memory_t *m = wasm_memory_new(store, mtype);
  wasm_memorytype_delete(mtype);

  // Cross-conversion grid: kind + round-trip.
  wasm_extern_t *ef = wasm_func_as_extern(f);
  wasm_extern_t *ef_fin = wasm_func_as_extern(f_fin);
  wasm_extern_t *eg = wasm_global_as_extern(g);
  wasm_extern_t *et = wasm_table_as_extern(t);
  wasm_extern_t *em = wasm_memory_as_extern(m);
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_extern_kind(ef));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_extern_kind(eg));
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_extern_kind(et));
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_extern_kind(em));
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_extern_kind(nullptr));
  EXPECT_EQ(f, wasm_extern_as_func(ef));
  EXPECT_EQ(g, wasm_extern_as_global(eg));
  EXPECT_EQ(t, wasm_extern_as_table(et));
  EXPECT_EQ(m, wasm_extern_as_memory(em));

  // Const variants.
  const wasm_extern_t *cef = wasm_func_as_extern_const(f);
  const wasm_extern_t *ceg = wasm_global_as_extern_const(g);
  const wasm_extern_t *cet = wasm_table_as_extern_const(t);
  const wasm_extern_t *cem = wasm_memory_as_extern_const(m);
  EXPECT_EQ(f, wasm_extern_as_func_const(cef));
  EXPECT_EQ(g, wasm_extern_as_global_const(ceg));
  EXPECT_EQ(t, wasm_extern_as_table_const(cet));
  EXPECT_EQ(m, wasm_extern_as_memory_const(cem));

  // Null guards on every cast.
  EXPECT_EQ(nullptr, wasm_func_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_global_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_table_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_memory_as_extern(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_func(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_global(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_table(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_memory(nullptr));
  EXPECT_EQ(nullptr, wasm_func_as_extern_const(nullptr));
  EXPECT_EQ(nullptr, wasm_global_as_extern_const(nullptr));
  EXPECT_EQ(nullptr, wasm_table_as_extern_const(nullptr));
  EXPECT_EQ(nullptr, wasm_memory_as_extern_const(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_func_const(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_global_const(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_table_const(nullptr));
  EXPECT_EQ(nullptr, wasm_extern_as_memory_const(nullptr));

  // wasm_extern_type returns the right kind for each.
  wasm_externtype_t *etf = wasm_extern_type(ef);
  wasm_externtype_t *etg = wasm_extern_type(eg);
  wasm_externtype_t *ett = wasm_extern_type(et);
  wasm_externtype_t *etm = wasm_extern_type(em);
  EXPECT_EQ(WASM_EXTERN_FUNC, wasm_externtype_kind(etf));
  EXPECT_EQ(WASM_EXTERN_GLOBAL, wasm_externtype_kind(etg));
  EXPECT_EQ(WASM_EXTERN_TABLE, wasm_externtype_kind(ett));
  EXPECT_EQ(WASM_EXTERN_MEMORY, wasm_externtype_kind(etm));
  wasm_externtype_delete(etf);
  wasm_externtype_delete(etg);
  wasm_externtype_delete(ett);
  wasm_externtype_delete(etm);

  WASM_DECLARE_REF_TEST(extern, ef, ef_fin)

  // Vec ops over a mixed wasm_extern_t * array. WASM_DECLARE_VEC_TEST takes
  // ownership of vals[i], so reborrow each X via wasm_X_copy first to avoid
  // double-frees against the original f/g/t/m.
  wasm_extern_t *exts[4] = {wasm_func_as_extern(wasm_func_copy(f)),
                            wasm_global_as_extern(wasm_global_copy(g)),
                            wasm_table_as_extern(wasm_table_copy(t)),
                            wasm_memory_as_extern(wasm_memory_copy(m))};
  wasm_extern_vec_t out, copy;
  WASM_DECLARE_VEC_TEST(extern, 4, exts, out, copy)
  WASM_DECLARE_VEC_DEL_TEST(extern, out, copy)

  // Clean up the originals.
  wasm_memory_delete(m);
  wasm_table_delete(t);
  wasm_global_delete(g);
  wasm_func_delete(f);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

TEST(APIWasmCTest, Ref) {
  wasm_engine_t *engine = wasm_engine_new();
  wasm_store_t *store = wasm_store_new(engine);

  // Lift a wasm_foreign_t to a wasm_ref_t and exercise the generic ref API.
  wasm_foreign_t *foreign = wasm_foreign_new(store);
  ASSERT_NE(nullptr, foreign);
  wasm_ref_t *ref = wasm_foreign_as_ref(foreign);
  ASSERT_NE(nullptr, ref);
  wasm_foreign_t *foreign_fin = wasm_foreign_new(store);
  ASSERT_NE(nullptr, foreign_fin);
  wasm_ref_t *ref_fin = wasm_foreign_as_ref(foreign_fin);
  ASSERT_NE(nullptr, ref_fin);

  WASM_DECLARE_REF_BASE_TEST(ref, ref, ref_fin)

  // wasm_ref_delete null guard.
  wasm_ref_delete(nullptr);

  wasm_foreign_delete(foreign);
  wasm_store_delete(store);
  wasm_engine_delete(engine);
  EXPECT_TRUE(true);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
