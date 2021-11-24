// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#include "common/configure.h"
#include "common/errcode.h"
#include "common/log.h"
#include "common/span.h"

#include "aot/compiler.h"
#include "ast/module.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"

#include "wasm/wasm.h"
#include "wasm/wasm.hh"
#include "wasmedge/wasmedge.h"

#ifndef OWN
#define OWN
#endif

/// wasm_config_t implementation.
struct wasm_config_t : public wasm::Config {
  wasm_config_t() noexcept = default;
  ~wasm_config_t() noexcept = default;
  WasmEdge::Configure conf;
};

/// wasm_engine_t implementation.
struct wasm_engine_t : public wasm::Engine {
  wasm_engine_t() noexcept : conf(), exec(conf, nullptr) {}
  wasm_engine_t(const wasm_config_t *c) noexcept
      : conf(c->conf), exec(c->conf, nullptr) {}
  ~wasm_engine_t() noexcept = default;
  WasmEdge::Configure conf;
  WasmEdge::Executor::Executor exec;
};

/// wasm_store_t implementation.
struct wasm_store_t : public wasm::Store {
  wasm_store_t(wasm_engine_t *e) noexcept
      : engine(e), load(e->conf), valid(e->conf), store() {}
  ~wasm_store_t() noexcept = default;
  wasm_engine_t *engine;
  WasmEdge::Loader::Loader load;
  WasmEdge::Validator::Validator valid;
  WasmEdge::Runtime::StoreManager store;
};

/// wasm_valtype_t implementation.
struct wasm_valtype_t : public wasm::ValType {
  wasm_valtype_t(wasm::ValKind k) noexcept : mkind(k) {}
  ~wasm_valtype_t() noexcept = default;
  wasm::ValKind mkind;
};

/// wasm_externtype_kind_t implementation.
struct wasm_externtype_kind_t {
  wasm_externtype_kind_t(wasm::ExternKind k) noexcept : mkind(k) {}
  wasm::ExternKind mkind;
};

/// wasm_externtype_t implementation.
struct wasm_externtype_t : public wasm::ExternType, wasm_externtype_kind_t {};

/// wasm_externtype_impl_t implementation.
template <class C>
struct wasm_externtype_impl_t : public C, wasm_externtype_kind_t {
  wasm_externtype_impl_t(wasm::ExternKind k) noexcept
      : wasm_externtype_kind_t(k) {}
};

/// wasm_functype_t implementation.
struct wasm_functype_t : public wasm_externtype_impl_t<wasm::FuncType> {
  wasm_functype_t(wasm::ownvec<wasm::ValType> &&vp,
                  wasm::ownvec<wasm::ValType> &&vr) noexcept
      : wasm_externtype_impl_t(wasm::ExternKind::FUNC), mparams(std::move(vp)),
        mresults(std::move(vr)) {}
  ~wasm_functype_t() noexcept = default;
  wasm::ownvec<wasm::ValType> mparams, mresults;
};

/// wasm_globaltype_t implementation.
struct wasm_globaltype_t : public wasm_externtype_impl_t<wasm::GlobalType> {
  wasm_globaltype_t(wasm::own<wasm::ValType> &&vt,
                    wasm::Mutability mut) noexcept
      : wasm_externtype_impl_t(wasm::ExternKind::GLOBAL),
        mcontent(std::move(vt)), mmutability(mut) {}
  ~wasm_globaltype_t() noexcept = default;
  wasm::own<wasm::ValType> mcontent;
  wasm::Mutability mmutability;
};

/// wasm_tabletype_t implementation.
struct wasm_tabletype_t : public wasm_externtype_impl_t<wasm::TableType> {
  wasm_tabletype_t(wasm::own<wasm::ValType> &&vt, wasm::Limits lim) noexcept
      : wasm_externtype_impl_t(wasm::ExternKind::TABLE),
        mvaltype(std::move(vt)), mlimits(lim) {}
  ~wasm_tabletype_t() noexcept = default;
  wasm::own<wasm::ValType> mvaltype;
  wasm::Limits mlimits;
};

/// wasm_memorytype_t implementation.
struct wasm_memorytype_t : public wasm_externtype_impl_t<wasm::MemoryType> {
  wasm_memorytype_t(wasm::Limits lim) noexcept
      : wasm_externtype_impl_t(wasm::ExternKind::MEMORY), mlimits(lim) {}
  ~wasm_memorytype_t() noexcept = default;
  wasm::Limits mlimits;
};

/// wasm_importtype_t implementation.
struct wasm_importtype_t : public wasm::ImportType {
  wasm_importtype_t(wasm::Name &&mod, wasm::Name &&n,
                    wasm::own<wasm::ExternType> &&ext) noexcept
      : mmodname(std::move(mod)), mname(std::move(n)), mtype(std::move(ext)) {}
  ~wasm_importtype_t() noexcept = default;
  wasm::Name mmodname, mname;
  wasm::own<wasm::ExternType> mtype;
};

/// wasm_exporttype_t implementation.
struct wasm_exporttype_t : public wasm::ExportType {
  wasm_exporttype_t(wasm::Name &&n, wasm::own<wasm::ExternType> &&ext) noexcept
      : mname(std::move(n)), mtype(std::move(ext)) {}
  ~wasm_exporttype_t() noexcept = default;
  wasm::Name mname;
  wasm::own<wasm::ExternType> mtype;
};

namespace {

/// Global variables to prevent from new/delete.
wasm_valtype_t wasm_valtype_i32(wasm::ValKind::I32);
wasm_valtype_t wasm_valtype_i64(wasm::ValKind::I64);
wasm_valtype_t wasm_valtype_f32(wasm::ValKind::F32);
wasm_valtype_t wasm_valtype_f64(wasm::ValKind::F64);
wasm_valtype_t wasm_valtype_anyref(wasm::ValKind::ANYREF);
wasm_valtype_t wasm_valtype_funcref(wasm::ValKind::FUNCREF);

/// Helper macro for the C APIs to get attributtes.
#define WASM_DECLARE_C_GET_ATTR(name, target, attr, cast_type, def_val,        \
                                is_ref, is_const)                              \
  WASMEDGE_CAPI_EXPORT is_const target wasm_##name##_##attr(                   \
      const wasm_##name##_t *name) {                                           \
    if (name) {                                                                \
      return cast_type##_cast<is_const target>(is_ref(name->attr()));          \
    }                                                                          \
    return def_val;                                                            \
  }

/// Helper macro for the WASM_DECLARE_OWN implementation.
#define WASM_DECLARE_OWN_IMPL(name, Name)                                      \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_delete(OWN wasm_##name##_t *obj) {   \
    wasm::own<Name> ptr(obj);                                                  \
  }

#define WASM_DECLARE_VEC_BASE_IMPL(name, Name)                                 \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_uninitialized(               \
      wasm_##name##_vec_t *out, size_t size) {                                 \
    wasm::vec<Name> v1 = wasm::vec<Name>::make_uninitialized(size);            \
    *out = {v1.size(), reinterpret_cast<wasm_val_t *>(v1.release())};          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_empty(                       \
      wasm_##name##_vec_t *out) {                                              \
    wasm_##name##_vec_new_uninitialized(out, 0);                               \
  }                                                                            \
                                                                               \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_delete(wasm_##name##_vec_t *v) { \
    wasm::vec<Name>::adopt(                                                    \
        v->size, reinterpret_cast<wasm::vec<Name>::elem_type *>(v->data));     \
  }

/// Helper macro for the parts of WASM_DECLARE_VEC implementation.
#define WASM_DECLARE_VEC_SCALAR_IMPL(name, Name)                               \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new(                             \
      OWN wasm_##name##_vec_t *out, size_t size,                               \
      OWN wasm_##name##_t const vals[]) {                                      \
    if (out) {                                                                 \
      auto v = wasm::vec<Name>::make_uninitialized(size);                      \
      if (v.size() > 0) {                                                      \
        std::copy_n(vals, size, v.get());                                      \
      }                                                                        \
      *out = {v.size(), v.release()};                                          \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_uninitialized(               \
      OWN wasm_##name##_vec_t *out, size_t size) {                             \
    if (out) {                                                                 \
      auto v = wasm::vec<Name>::make_uninitialized(size);                      \
      *out = {v.size(), v.release()};                                          \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_empty(                       \
      OWN wasm_##name##_vec_t *out) {                                          \
    wasm_##name##_vec_new_uninitialized(out, 0);                               \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_copy(                            \
      OWN wasm_##name##_vec_t *out, const wasm_##name##_vec_t *in) {           \
    if (in && out) {                                                           \
      wasm_##name##_vec_new(out, in->size, in->data);                          \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_delete(                          \
      OWN wasm_##name##_vec_t *vec) {                                          \
    if (vec) {                                                                 \
      wasm::vec<Name>::adopt(                                                  \
          vec->size,                                                           \
          reinterpret_cast<wasm::vec<Name>::elem_type *>(vec->data));          \
      vec->size = 0;                                                           \
      vec->data = nullptr;                                                     \
    }                                                                          \
  }

/// Helper macro for the parts of WASM_DECLARE_VEC implementation.
#define WASM_DECLARE_VEC_PTR_IMPL(name, Name)                                  \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new(                             \
      OWN wasm_##name##_vec_t *out, size_t size,                               \
      OWN wasm_##name##_t *const data[]) {                                     \
    if (out) {                                                                 \
      auto v = wasm::ownvec<Name>::make_uninitialized(size);                   \
      for (size_t i = 0; i < v.size(); ++i) {                                  \
        v[i] = wasm::make_own(data[i]);                                        \
      }                                                                        \
      *out = {v.size(), reinterpret_cast<wasm_##name##_t **>(v.release())};    \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_uninitialized(               \
      OWN wasm_##name##_vec_t *out, size_t size) {                             \
    if (out) {                                                                 \
      auto v = wasm::ownvec<Name>::make_uninitialized(size);                   \
      *out = {v.size(), reinterpret_cast<wasm_##name##_t **>(v.release())};    \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_empty(                       \
      OWN wasm_##name##_vec_t *out) {                                          \
    wasm_##name##_vec_new_uninitialized(out, 0);                               \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_copy(                            \
      OWN wasm_##name##_vec_t *out, const wasm_##name##_vec_t *in) {           \
    if (in && out) {                                                           \
      auto v = wasm::ownvec<Name>::make_uninitialized(in->size);               \
      for (size_t i = 0; i < v.size(); ++i) {                                  \
        v[i] = wasm::make_own(wasm_##name##_copy(in->data[i]));                \
      }                                                                        \
      *out = {v.size(), reinterpret_cast<wasm_##name##_t **>(v.release())};    \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_delete(                          \
      OWN wasm_##name##_vec_t *vec) {                                          \
    if (vec) {                                                                 \
      wasm::ownvec<Name>::adopt(                                               \
          vec->size, reinterpret_cast<wasm::own<Name> *>(vec->data));          \
      vec->size = 0;                                                           \
      vec->data = nullptr;                                                     \
    }                                                                          \
  }

/// Helper macro for the WASM_DECLARE_TYPE implementation.
#define WASM_DECLARE_TYPE_IMPL(name, Name)                                     \
  WASM_DECLARE_OWN_IMPL(name, Name)                                            \
  WASM_DECLARE_VEC_PTR_IMPL(name, Name)                                        \
  WASMEDGE_CAPI_EXPORT wasm_##name##_t *wasm_##name##_copy(                    \
      const wasm_##name##_t *inst) {                                           \
    if (inst) {                                                                \
      return static_cast<wasm_##name##_t *>((inst->copy()).release());         \
    }                                                                          \
    return nullptr;                                                            \
  }

/// Helper macro for the wasm_externtype_t conversions.
#define WASM_EXTERNTYPE_C_CONV(in, out)                                        \
  WASMEDGE_CAPI_EXPORT wasm_##out##_t *wasm_##in##_as_##out(                   \
      wasm_##in##_t *in) {                                                     \
    return static_cast<wasm_##out##_t *>(static_cast<wasm::ExternType *>(in)); \
  }
#define WASM_EXTERNTYPE_C_CONV_CONST(in, out)                                  \
  WASMEDGE_CAPI_EXPORT const wasm_##out##_t *wasm_##in##_as_##out##_const(     \
      const wasm_##in##_t *in) {                                               \
    return static_cast<const wasm_##out##_t *>(                                \
        static_cast<const wasm::ExternType *>(in));                            \
  }

/// Helper macro for the wasm::ExternType conversions.
#define WASM_EXTERNTYPE_CPP_CONV(name, Name, enum, const_quant)                \
  const_quant Name *wasm::ExternType::name() const_quant {                     \
    if (kind() == wasm::ExternKind::enum) {                                    \
      return static_cast<const_quant Name *>(this);                            \
    }                                                                          \
    return nullptr;                                                            \
  }

} // namespace

/// The followings are the C API implementation.
#ifdef __cplusplus
extern "C" {
#endif

/// >>>>>>>> wasm_byte_vec_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_VEC_SCALAR_IMPL(byte, byte_t)

/// <<<<<<<< wasm_byte_vec_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_config_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(config, wasm::Config)

WASMEDGE_CAPI_EXPORT OWN wasm_config_t *wasm_config_new() {
  return new (std::nothrow) wasm_config_t;
}

/// <<<<<<<< wasm_config_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_engine_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(engine, wasm::Engine)

WASMEDGE_CAPI_EXPORT OWN wasm_engine_t *wasm_engine_new() {
  return new (std::nothrow) wasm_engine_t();
}

WASMEDGE_CAPI_EXPORT OWN wasm_engine_t *
wasm_engine_new_with_config(OWN wasm_config_t *conf) {
  wasm_engine_t *res = new (std::nothrow) wasm_engine_t(conf);
  wasm_config_delete(conf);
  return res;
}

/// <<<<<<<< wasm_engine_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_store_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(store, wasm::Store)

WASMEDGE_CAPI_EXPORT OWN wasm_store_t *wasm_store_new(wasm_engine_t *engine) {
  if (engine) {
    return new (std::nothrow) wasm_store_t(engine);
  }
  return nullptr;
}

/// <<<<<<<< wasm_store_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_valtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(valtype, wasm::ValType)

WASMEDGE_CAPI_EXPORT OWN wasm_valtype_t *wasm_valtype_new(wasm_valkind_t kind) {
  /// Return the pointer to the instances.
  /// When delete the `wasm_valtype_t`, the `destroy()` function will do nothing
  /// when the unique pointer releases the pointer.
  switch (kind) {
  case WASM_I32:
    return &wasm_valtype_i32;
  case WASM_I64:
    return &wasm_valtype_i64;
  case WASM_F32:
    return &wasm_valtype_f32;
  case WASM_F64:
    return &wasm_valtype_f64;
  case WASM_ANYREF:
    return &wasm_valtype_anyref;
  case WASM_FUNCREF:
    return &wasm_valtype_funcref;
  };
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(valtype, wasm_valkind_t, kind, static, WASM_I32, , )

/// <<<<<<<< wasm_valtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_functype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(functype, wasm::FuncType)

WASMEDGE_CAPI_EXPORT OWN wasm_functype_t *
wasm_functype_new(OWN wasm_valtype_vec_t *params,
                  OWN wasm_valtype_vec_t *results) {
  if (params && results) {
    return new (std::nothrow) wasm_functype_t(
        wasm::ownvec<wasm::ValType>::adopt(
            params->size,
            reinterpret_cast<wasm::own<wasm::ValType> *>(params->data)),
        wasm::ownvec<wasm::ValType>::adopt(
            results->size,
            reinterpret_cast<wasm::own<wasm::ValType> *>(results->data)));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(functype, wasm_valtype_vec_t *, params, reinterpret,
                        nullptr, &, const)
WASM_DECLARE_C_GET_ATTR(functype, wasm_valtype_vec_t *, results, reinterpret,
                        nullptr, &, const)

/// <<<<<<<< wasm_functype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_globaltype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(globaltype, wasm::GlobalType)

WASMEDGE_CAPI_EXPORT OWN wasm_globaltype_t *
wasm_globaltype_new(OWN wasm_valtype_t *valtype, wasm_mutability_t mutability) {
  if (valtype) {
    return new (std::nothrow) wasm_globaltype_t(
        wasm::make_own(valtype), static_cast<wasm::Mutability>(mutability));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(globaltype, wasm_valtype_t *, content, static, nullptr,
                        , const)
WASM_DECLARE_C_GET_ATTR(globaltype, wasm_mutability_t, mutability, static,
                        WASM_CONST, , )

/// <<<<<<<< wasm_globaltype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_tabletype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(tabletype, wasm::TableType)

WASMEDGE_CAPI_EXPORT OWN wasm_tabletype_t *
wasm_tabletype_new(OWN wasm_valtype_t *valtype, const wasm_limits_t *limits) {
  if (valtype && limits) {
    return new (std::nothrow) wasm_tabletype_t(
        wasm::make_own(valtype), wasm::Limits(limits->min, limits->max));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(tabletype, wasm_valtype_t *, element, static, nullptr, ,
                        const)
WASM_DECLARE_C_GET_ATTR(tabletype, wasm_limits_t *, limits, reinterpret,
                        nullptr, &, const)

/// <<<<<<<< wasm_tabletype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_memorytype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(memorytype, wasm::MemoryType)

WASMEDGE_CAPI_EXPORT OWN wasm_memorytype_t *
wasm_memorytype_new(const wasm_limits_t *limits) {
  if (limits) {
    return new (std::nothrow)
        wasm_memorytype_t(wasm::Limits(limits->min, limits->max));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(memorytype, wasm_limits_t *, limits, reinterpret,
                        nullptr, &, const)

/// <<<<<<<< wasm_memorytype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_externtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(externtype, wasm::ExternType)

WASMEDGE_CAPI_EXPORT wasm_externkind_t
wasm_externtype_kind(const wasm_externtype_t *externtype) {
  if (externtype) {
    return static_cast<wasm_externkind_t>(
        (static_cast<const wasm_externtype_kind_t *>(externtype))->mkind);
  }
  return WASM_EXTERN_FUNC;
}

WASM_EXTERNTYPE_C_CONV(functype, externtype)
WASM_EXTERNTYPE_C_CONV(globaltype, externtype)
WASM_EXTERNTYPE_C_CONV(tabletype, externtype)
WASM_EXTERNTYPE_C_CONV(memorytype, externtype)
WASM_EXTERNTYPE_C_CONV(externtype, functype)
WASM_EXTERNTYPE_C_CONV(externtype, globaltype)
WASM_EXTERNTYPE_C_CONV(externtype, tabletype)
WASM_EXTERNTYPE_C_CONV(externtype, memorytype)
WASM_EXTERNTYPE_C_CONV_CONST(functype, externtype)
WASM_EXTERNTYPE_C_CONV_CONST(globaltype, externtype)
WASM_EXTERNTYPE_C_CONV_CONST(tabletype, externtype)
WASM_EXTERNTYPE_C_CONV_CONST(memorytype, externtype)
WASM_EXTERNTYPE_C_CONV_CONST(externtype, functype)
WASM_EXTERNTYPE_C_CONV_CONST(externtype, globaltype)
WASM_EXTERNTYPE_C_CONV_CONST(externtype, tabletype)
WASM_EXTERNTYPE_C_CONV_CONST(externtype, memorytype)

/// <<<<<<<< wasm_externtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_importtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(importtype, wasm::ImportType)

WASMEDGE_CAPI_EXPORT OWN wasm_importtype_t *
wasm_importtype_new(OWN wasm_name_t *module, OWN wasm_name_t *name,
                    OWN wasm_externtype_t *externtype) {
  if (module && name && externtype) {
    return new (std::nothrow)
        wasm_importtype_t(wasm::vec<byte_t>::adopt(module->size, module->data),
                          wasm::vec<byte_t>::adopt(name->size, name->data),
                          wasm::make_own(externtype));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(importtype, wasm_name_t *, module, reinterpret,
                        nullptr, &, const)
WASM_DECLARE_C_GET_ATTR(importtype, wasm_name_t *, name, reinterpret,
                        nullptr, &, const)
WASM_DECLARE_C_GET_ATTR(importtype, wasm_externtype_t *, type, static, nullptr,
                        , const)

/// <<<<<<<< wasm_importtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm_exporttype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(exporttype, wasm::ExportType)

WASMEDGE_CAPI_EXPORT OWN wasm_exporttype_t *
wasm_exporttype_new(OWN wasm_name_t *name, OWN wasm_externtype_t *externtype) {
  if (name && externtype) {
    return new (std::nothrow)
        wasm_exporttype_t(wasm::vec<byte_t>::adopt(name->size, name->data),
                          wasm::make_own(externtype));
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(exporttype, wasm_name_t *, name, reinterpret,
                        nullptr, &, const)
WASM_DECLARE_C_GET_ATTR(exporttype, wasm_externtype_t *, type, static, nullptr,
                        , const)

/// <<<<<<<< wasm_exporttype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define UNUSED(x) (void)(x)

///////////////////////////////////////////////////////////////////////////////
// Runtime Values

// References

struct wasm_ref_t : public wasm::Ref {};

#define WASM_DEFINE_REF_BASE(name, Name)                                       \
  WASM_DECLARE_OWN_IMPL(name, Name)                                            \
                                                                               \
  WASMEDGE_CAPI_EXPORT bool wasm_##name##_same(const wasm_##name##_t *t1,      \
                                               const wasm_##name##_t *t2) {    \
    return t1->same(t2);                                                       \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT wasm_##name##_t *wasm_##name##_copy(                    \
      const wasm_##name##_t *t) {                                              \
    return static_cast<wasm_##name##_t *>(t->copy().release());                \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void *wasm_##name##_get_host_info(                      \
      const wasm_##name##_t *r) {                                              \
    return r->get_host_info();                                                 \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_set_host_info(wasm_##name##_t *r,    \
                                                        void *info) {          \
    r->set_host_info(info);                                                    \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_set_host_info_with_finalizer(        \
      wasm_##name##_t *r, void *info, void (*finalizer)(void *)) {             \
    r->set_host_info(info, finalizer);                                         \
  }

#define WASM_DEFINE_REF(name, Name)                                            \
  WASM_DEFINE_REF_BASE(name, Name)                                             \
                                                                               \
  WASMEDGE_CAPI_EXPORT wasm_ref_t *wasm_##name##_as_ref(wasm_##name##_t *r) {  \
    return static_cast<wasm_ref_t *>(                                          \
        static_cast<wasm::Ref *>(static_cast<Name *>(r)));                     \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT wasm_##name##_t *wasm_ref_as_##name(wasm_ref_t *r) {    \
    return static_cast<wasm_##name##_t *>(                                     \
        static_cast<Name *>(static_cast<wasm::Ref *>(r)));                     \
  }                                                                            \
                                                                               \
  WASMEDGE_CAPI_EXPORT const wasm_ref_t *wasm_##name##_as_ref_const(           \
      const wasm_##name##_t *r) {                                              \
    return static_cast<const wasm_ref_t *>(                                    \
        static_cast<const wasm::Ref *>(static_cast<const Name *>(r)));         \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT const wasm_##name##_t *wasm_ref_as_##name##_const(      \
      const wasm_ref_t *r) {                                                   \
    return static_cast<const wasm_##name##_t *>(                               \
        static_cast<const Name *>(static_cast<const wasm::Ref *>(r)));         \
  }

#define WASM_DEFINE_SHARABLE_REF(name, Name)                                   \
  WASM_DEFINE_REF(name, Name)                                                  \
  WASM_DECLARE_OWN_IMPL(shared_##name, wasm::Shared<wasm::Module>)

WASM_DEFINE_REF_BASE(ref, wasm::Ref)

// Values

extern "C++" {

inline bool is_empty(wasm_val_t v) {
  return !is_ref(static_cast<wasm::ValKind>(v.kind)) || !v.of.ref;
}
// TODO: Find solution to:
// missing initializer for member ‘wasm_val_t::of’
// [-Werror=missing-field-initializers]

inline wasm_val_t hide_val(wasm::Val v) {
  //  wasm_val_t v2 = {static_cast<wasm_valkind_t>(v.kind())};
  wasm_valkind_t kind = static_cast<wasm_valkind_t>(v.kind());
  switch (v.kind()) {
  case wasm::ValKind::I32:
    return {kind, {.i32 = v.i32()}};
  case wasm::ValKind::I64:
    return {kind, {.i64 = v.i64()}};
  case wasm::ValKind::F32:
    return {kind, {.f32 = v.f32()}};
  case wasm::ValKind::F64:
    return {kind, {.f64 = v.f64()}};
  case wasm::ValKind::ANYREF:
  case wasm::ValKind::FUNCREF:
    return {kind, {.ref = static_cast<wasm_ref_t *>(v.ref())}};
  default:
    break;
    assert(false);
  }
  return {kind, {.i32 = v.i32()}};
}

inline wasm_val_t release_val(wasm::Val v) {
  //  wasm_val_t v2 = {static_cast<wasm_valkind_t>(v.kind()), {v.i32()}};
  wasm_valkind_t kind = static_cast<wasm_valkind_t>(v.kind());
  switch (v.kind()) {
  case wasm::ValKind::I32:
    return {kind, {.i32 = v.i32()}};
  case wasm::ValKind::I64:
    return {kind, {.i64 = v.i64()}};
  case wasm::ValKind::F32:
    return {kind, {.f32 = v.f32()}};
  case wasm::ValKind::F64:
    return {kind, {.f64 = v.f64()}};
  case wasm::ValKind::ANYREF:
  case wasm::ValKind::FUNCREF:
    return {kind,
            {.ref = static_cast<wasm_ref_t *>(v.release_ref().release())}};
  default:
    assert(false);
  }
  return {kind, {.i32 = v.i32()}};
}

inline wasm::Val adopt_val(wasm_val_t v) {
  switch (static_cast<wasm::ValKind>(v.kind)) {
  case wasm::ValKind::I32:
    return wasm::Val(v.of.i32);
  case wasm::ValKind::I64:
    return wasm::Val(v.of.i64);
  case wasm::ValKind::F32:
    return wasm::Val(v.of.f32);
  case wasm::ValKind::F64:
    return wasm::Val(v.of.f64);
  case wasm::ValKind::ANYREF:
  case wasm::ValKind::FUNCREF:
    return wasm::Val(wasm::make_own(v.of.ref));
  default:
    assert(false);
  }
  return wasm::Val(v.of.i32);
}

struct borrowed_val {
  wasm::Val it;

  borrowed_val(wasm::Val &&v) : it(std::move(v)) {}

  borrowed_val(borrowed_val &&that) : it(std::move(that.it)) {}

  ~borrowed_val() {
    if (it.is_ref())
      it.release_ref().release();
  }
};

inline borrowed_val borrow_val(const wasm_val_t *v) {
  wasm::Val v2;
  switch (static_cast<wasm::ValKind>(v->kind)) {
  case wasm::ValKind::I32:
    v2 = wasm::Val(v->of.i32);
    break;
  case wasm::ValKind::I64:
    v2 = wasm::Val(v->of.i64);
    break;
  case wasm::ValKind::F32:
    v2 = wasm::Val(v->of.f32);
    break;
  case wasm::ValKind::F64:
    v2 = wasm::Val(v->of.f64);
    break;
  case wasm::ValKind::ANYREF:
  case wasm::ValKind::FUNCREF:
    v2 = wasm::Val(make_own(v->of.ref));
    break;
  default:
    assert(false);
  }
  return borrowed_val(std::move(v2));
}

} // extern "C++"

// WASM_DECLARE_VEC_SCALAR_IMPL(val, wasm::Val);
WASM_DECLARE_VEC_BASE_IMPL(val, wasm::Val);
// WASM_DEFINE_VEC_BASE(val, Val, vec, vec, )

WASMEDGE_CAPI_EXPORT void wasm_val_vec_new(wasm_val_vec_t *out, size_t size,
                                           wasm_val_t const data[]) {
  auto v2 = wasm::vec<wasm::Val>::make_uninitialized(size);
  for (size_t i = 0; i < v2.size(); ++i) {
    v2[i] = adopt_val(data[i]);
  }
  *out = {v2.size(), reinterpret_cast<wasm_val_t *>(v2.release())};
}

WASMEDGE_CAPI_EXPORT void wasm_val_vec_copy(wasm_val_vec_t *out,
                                            const wasm_val_vec_t *v) {
  auto v2 = wasm::vec<wasm::Val>::make_uninitialized(v->size);
  for (size_t i = 0; i < v2.size(); ++i) {
    wasm_val_t val;
    wasm_val_copy(&v->data[i], &val);
    v2[i] = adopt_val(val);
  }
  *out = {v2.size(), reinterpret_cast<wasm_val_t *>(v2.release())};
  //      release_val_vec(std::move(v2));
}

WASMEDGE_CAPI_EXPORT void wasm_val_delete(wasm_val_t *v) {
  if (is_ref(static_cast<wasm::ValKind>(v->kind))) {
    wasm::make_own(v->of.ref);
  }
}

WASMEDGE_CAPI_EXPORT void wasm_val_copy(wasm_val_t *out, const wasm_val_t *v) {
  *out = *v;
  if (is_ref(static_cast<wasm::ValKind>(v->kind))) {
    out->of.ref = v->of.ref
                      ? static_cast<wasm_ref_t *>(v->of.ref->copy().release())
                      : nullptr;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Runtime Objects

// Frames
struct wasm_frame_t : public wasm::Frame {};
// 多了幾個 function
// 在拆開
WASM_DECLARE_OWN_IMPL(frame, wasm::Frame)
WASM_DECLARE_VEC_PTR_IMPL(frame, wasm::Frame)

WASMEDGE_CAPI_EXPORT wasm_frame_t *wasm_frame_copy(const wasm_frame_t *frame) {
  return static_cast<wasm_frame_t *>(frame->copy().release());
}

WASMEDGE_CAPI_EXPORT wasm_instance_t *
wasm_frame_instance(const wasm_frame_t *frame);
// Defined below along with wasm_instance_t.

WASMEDGE_CAPI_EXPORT uint32_t wasm_frame_func_index(const wasm_frame_t *frame) {
  return static_cast<const wasm::Frame *>(frame)->func_index();
}

WASMEDGE_CAPI_EXPORT size_t wasm_frame_func_offset(const wasm_frame_t *frame) {
  return static_cast<const wasm::Frame *>(frame)->func_offset();
}

WASMEDGE_CAPI_EXPORT size_t
wasm_frame_module_offset(const wasm_frame_t *frame) {
  return static_cast<const wasm::Frame *>(frame)->module_offset();
}

// Traps

/// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

struct wasm_trap_t : wasm::Trap {};

WASM_DEFINE_REF(trap, wasm::Trap)

WASMEDGE_CAPI_EXPORT wasm_trap_t *wasm_trap_new(wasm_store_t *store,
                                                const wasm_message_t *message) {
  UNUSED(store);
  UNUSED(message);
  // 可用 stl
  // auto message_ = borrow_byte_vec(message);
  // return release_trap(Trap::make(store, message_.it));X
  return static_cast<wasm_trap_t *>(nullptr);
}

WASMEDGE_CAPI_EXPORT void wasm_trap_message(const wasm_trap_t *trap,
                                            wasm_message_t *out) {
  UNUSED(trap);
  UNUSED(out);
  //*out = release_byte_vec(static_cast<const wasm::Trap *>(trap)->message());
}

WASMEDGE_CAPI_EXPORT wasm_frame_t *wasm_trap_origin(const wasm_trap_t *trap) {
  UNUSED(trap);
  // return release_frame(static_cast<const wasm::Trap *>(trap)->origin());
  return static_cast<wasm_frame_t *>(nullptr);
}

WASMEDGE_CAPI_EXPORT void wasm_trap_trace(const wasm_trap_t *trap,
                                          wasm_frame_vec_t *out) {
  UNUSED(trap);
  UNUSED(out);
  //*out = release_frame_vec(static_cast<const wasm::Trap *>(trap)->trace());
}

// Foreign Objects

struct wasm_foreign_t : wasm::Foreign {};

WASM_DEFINE_REF(foreign, wasm::Foreign)

WASMEDGE_CAPI_EXPORT wasm_foreign_t *wasm_foreign_new(wasm_store_t *store) {
  return static_cast<wasm_foreign_t *>(wasm::Foreign::make(store).release());
}

// Modules

struct wasm_module_t : wasm::Module {};
struct wasm_shared_module_t : wasm::Shared<wasm::Module> {};
WASM_DEFINE_SHARABLE_REF(module, wasm::Module)

WASMEDGE_CAPI_EXPORT bool wasm_module_validate(wasm_store_t *store,
                                               const wasm_byte_vec_t *binary) {
  UNUSED(store);
  UNUSED(binary);
  //  auto binary_ = borrow_byte_vec(binary);
  //  return wasm::Module::validate(store, binary_.it);
  return 0;
}

WASMEDGE_CAPI_EXPORT wasm_module_t *
wasm_module_new(wasm_store_t *store, const wasm_byte_vec_t *binary) {
  UNUSED(store);
  UNUSED(binary);
  //  auto binary_ = borrow_byte_vec(binary);
  //  return release_module(Module::make(store, binary_.it));
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void wasm_module_imports(const wasm_module_t *module,
                                              wasm_importtype_vec_t *out) {
  UNUSED(module);
  UNUSED(out);
  //  *out = release_importtype_vec(reveal_module(module)->imports());
}

WASMEDGE_CAPI_EXPORT void wasm_module_exports(const wasm_module_t *module,
                                              wasm_exporttype_vec_t *out) {
  UNUSED(module);
  UNUSED(out);
  //  *out = release_exporttype_vec(reveal_module(module)->exports());
}

WASMEDGE_CAPI_EXPORT void wasm_module_serialize(const wasm_module_t *module,
                                                wasm_byte_vec_t *out) {
  UNUSED(module);
  UNUSED(out);
  //  *out = release_byte_vec(reveal_module(module)->serialize());
}

WASMEDGE_CAPI_EXPORT wasm_module_t *
wasm_module_deserialize(wasm_store_t *store, const wasm_byte_vec_t *binary) {
  UNUSED(store);
  UNUSED(binary);
  //  auto binary_ = borrow_byte_vec(binary);
  //  return release_module(Module::deserialize(store, binary_.it));
  return nullptr;
}

WASMEDGE_CAPI_EXPORT wasm_shared_module_t *
wasm_module_share(const wasm_module_t *module) {
  UNUSED(module);
  //  return release_shared_module(reveal_module(module)->share());
  return nullptr;
}

WASMEDGE_CAPI_EXPORT wasm_module_t *
wasm_module_obtain(wasm_store_t *store, const wasm_shared_module_t *shared) {
  UNUSED(store);
  UNUSED(shared);
  //  return release_module(Module::obtain(store, shared));
  return nullptr;
}

// Function Instances

struct wasm_func_t : wasm::Func {};
WASM_DEFINE_REF(func, wasm::Func)

extern "C++" {

wasm::own<wasm::Trap> wasm_callback(void *env, const wasm::vec<wasm::Val> &args,
                                    wasm::vec<wasm::Val> &results) {
  UNUSED(env);
  UNUSED(args);
  UNUSED(results);
  //  auto f = reinterpret_cast<wasm_func_callback_t>(env);
  //  return adopt_trap(f(hide_val_vec(args), hide_val_vec(results)));
  return nullptr;
}

struct wasm_callback_env_t {
  wasm_func_callback_with_env_t callback;
  void *env;

  void (*finalizer)(void *);
};

wasm::own<wasm::Trap> wasm_callback_with_env(void *env,
                                             const wasm::vec<wasm::Val> &args,
                                             wasm::vec<wasm::Val> &results) {
  auto t = static_cast<wasm_callback_env_t *>(env);
  UNUSED(t);
  UNUSED(args);
  UNUSED(results);
  //  return adopt_trap(
  //      t->callback(t->env, hide_val_vec(args), hide_val_vec(results)));
  return nullptr;
}

void wasm_callback_env_finalizer(void *env) {
  auto t = static_cast<wasm_callback_env_t *>(env);
  if (t->finalizer)
    t->finalizer(t->env);
  delete t;
}

} // extern "C++"

WASMEDGE_CAPI_EXPORT wasm_func_t *wasm_func_new(wasm_store_t *store,
                                                const wasm_functype_t *type,
                                                wasm_func_callback_t callback) {
  UNUSED(store);
  UNUSED(type);
  UNUSED(callback);
  // release_func
  //  return release_func(wasm::Func::make(
  //      store, type, wasm_callback, reinterpret_cast<void *>(callback)));
  return nullptr;
}

WASMEDGE_CAPI_EXPORT wasm_func_t *
wasm_func_new_with_env(wasm_store_t *store, const wasm_functype_t *type,
                       wasm_func_callback_with_env_t callback, void *env,
                       void (*finalizer)(void *)) {
  UNUSED(store);
  UNUSED(type);
  UNUSED(callback);
  UNUSED(env);
  UNUSED(finalizer);
  //  auto env2 = new wasm_callback_env_t{callback, env, finalizer};
  //  return release_func(wasm::Func::make(store, type, wasm_callback_with_env,
  //  env2, wasm_callback_env_finalizer));
  return nullptr;
}

WASMEDGE_CAPI_EXPORT wasm_functype_t *wasm_func_type(const wasm_func_t *func) {
  UNUSED(func);
  //  return release_functype(func->type());
  return nullptr;
}

WASMEDGE_CAPI_EXPORT size_t wasm_func_param_arity(const wasm_func_t *func) {
  return func->param_arity();
}

WASMEDGE_CAPI_EXPORT size_t wasm_func_result_arity(const wasm_func_t *func) {
  return func->result_arity();
}

WASMEDGE_CAPI_EXPORT wasm_trap_t *wasm_func_call(const wasm_func_t *func,
                                                 const wasm_val_vec_t *args,
                                                 wasm_val_vec_t *results) {
  UNUSED(func);
  UNUSED(args);
  UNUSED(results);
  //  auto args_ = borrow_val_vec(args);
  //  auto results_ = borrow_val_vec(results);
  //  return release_trap(func->call(args_.it, results_.it));
  return nullptr;
}

// Global Instances
struct wasm_global_t : wasm::Global {};
WASM_DEFINE_REF(global, wasm::Global)

WASMEDGE_CAPI_EXPORT wasm_global_t *
wasm_global_new(wasm_store_t *store, const wasm_globaltype_t *type,
                const wasm_val_t *val) {
  auto val_ = borrow_val(val);
  return static_cast<wasm_global_t *>(
      wasm::Global::make(store, type, val_.it).release());
}

WASMEDGE_CAPI_EXPORT wasm_globaltype_t *
wasm_global_type(const wasm_global_t *global) {
  return static_cast<wasm_globaltype_t *>(global->type().release());
}

WASMEDGE_CAPI_EXPORT void wasm_global_get(const wasm_global_t *global,
                                          wasm_val_t *out) {
  *out = release_val(global->get());
}

WASMEDGE_CAPI_EXPORT void wasm_global_set(wasm_global_t *global,
                                          const wasm_val_t *val) {
  auto val_ = borrow_val(val);
  global->set(val_.it);
}

// Table Instances
struct wasm_table_t : wasm::Table {};
WASM_DEFINE_REF(table, wasm::Table)

WASMEDGE_CAPI_EXPORT wasm_table_t *wasm_table_new(wasm_store_t *store,
                                                  const wasm_tabletype_t *type,
                                                  wasm_ref_t *ref) {
  return static_cast<wasm_table_t *>(
      wasm::Table::make(store, type, ref).release());
}

WASMEDGE_CAPI_EXPORT wasm_tabletype_t *
wasm_table_type(const wasm_table_t *table) {
  return static_cast<wasm_tabletype_t *>(table->type().release());
}

WASMEDGE_CAPI_EXPORT wasm_ref_t *wasm_table_get(const wasm_table_t *table,
                                                wasm_table_size_t index) {
  return static_cast<wasm_ref_t *>(table->get(index).release());
}

WASMEDGE_CAPI_EXPORT bool
wasm_table_set(wasm_table_t *table, wasm_table_size_t index, wasm_ref_t *ref) {
  return table->set(index, ref);
}

WASMEDGE_CAPI_EXPORT wasm_table_size_t
wasm_table_size(const wasm_table_t *table) {
  return table->size();
}

WASMEDGE_CAPI_EXPORT bool
wasm_table_grow(wasm_table_t *table, wasm_table_size_t delta, wasm_ref_t *ref) {
  return table->grow(delta, ref);
}

// Memory Instances
struct wasm_memory_t : wasm::Memory {};

WASM_DEFINE_REF(memory, wasm::Memory)

WASMEDGE_CAPI_EXPORT wasm_memory_t *
wasm_memory_new(wasm_store_t *store, const wasm_memorytype_t *type) {
  return static_cast<wasm_memory_t *>(
      wasm::Memory::make(store, type).release());
}

WASMEDGE_CAPI_EXPORT wasm_memorytype_t *
wasm_memory_type(const wasm_memory_t *memory) {
  return static_cast<wasm_memorytype_t *>(memory->type().release());
}

WASMEDGE_CAPI_EXPORT wasm_byte_t *wasm_memory_data(wasm_memory_t *memory) {
  return memory->data();
}

WASMEDGE_CAPI_EXPORT size_t wasm_memory_data_size(const wasm_memory_t *memory) {
  return memory->data_size();
}

WASMEDGE_CAPI_EXPORT wasm_memory_pages_t
wasm_memory_size(const wasm_memory_t *memory) {
  return memory->size();
}

WASMEDGE_CAPI_EXPORT bool wasm_memory_grow(wasm_memory_t *memory,
                                           wasm_memory_pages_t delta) {
  return memory->grow(delta);
}

// Externals
// WASM_DECLARE_TYPE_IMPL

struct wasm_extern_t : wasm::Extern {};

WASM_DEFINE_REF(extern, wasm::Extern)
WASM_DECLARE_VEC_PTR_IMPL(extern, wasm::Extern)

WASMEDGE_CAPI_EXPORT wasm_externkind_t
wasm_extern_kind(const wasm_extern_t *external) {
  return static_cast<wasm_externkind_t>(external->kind());
}
WASMEDGE_CAPI_EXPORT wasm_externtype_t *
wasm_extern_type(const wasm_extern_t *external) {
  return reinterpret_cast<wasm_externtype_t *>(external->type().release());
}

WASMEDGE_CAPI_EXPORT wasm_extern_t *wasm_func_as_extern(wasm_func_t *func) {
  return static_cast<wasm_extern_t *>(
      static_cast<wasm::Extern *>(static_cast<wasm::Func *>(func)));
}
WASMEDGE_CAPI_EXPORT wasm_extern_t *
wasm_global_as_extern(wasm_global_t *global) {
  return static_cast<wasm_extern_t *>(
      static_cast<wasm::Extern *>(static_cast<wasm::Global *>(global)));
}
WASMEDGE_CAPI_EXPORT wasm_extern_t *wasm_table_as_extern(wasm_table_t *table) {
  return static_cast<wasm_extern_t *>(
      static_cast<wasm::Extern *>(static_cast<wasm::Table *>(table)));
}
WASMEDGE_CAPI_EXPORT wasm_extern_t *
wasm_memory_as_extern(wasm_memory_t *memory) {
  return static_cast<wasm_extern_t *>(
      static_cast<wasm::Extern *>(static_cast<wasm::Memory *>(memory)));
}

WASMEDGE_CAPI_EXPORT const wasm_extern_t *
wasm_func_as_extern_const(const wasm_func_t *func) {
  return static_cast<const wasm_extern_t *>(
      static_cast<const wasm::Extern *>(static_cast<const wasm::Func *>(func)));
}
WASMEDGE_CAPI_EXPORT const wasm_extern_t *
wasm_global_as_extern_const(const wasm_global_t *global) {
  return static_cast<const wasm_extern_t *>(static_cast<const wasm::Extern *>(
      static_cast<const wasm::Global *>(global)));
}
WASMEDGE_CAPI_EXPORT const wasm_extern_t *
wasm_table_as_extern_const(const wasm_table_t *table) {
  return static_cast<const wasm_extern_t *>(static_cast<const wasm::Extern *>(
      static_cast<const wasm::Table *>(table)));
}
WASMEDGE_CAPI_EXPORT const wasm_extern_t *
wasm_memory_as_extern_const(const wasm_memory_t *memory) {
  return static_cast<const wasm_extern_t *>(static_cast<const wasm::Extern *>(
      static_cast<const wasm::Memory *>(memory)));
}

WASMEDGE_CAPI_EXPORT wasm_func_t *wasm_extern_as_func(wasm_extern_t *external) {
  return static_cast<wasm_func_t *>(external->func());
}
WASMEDGE_CAPI_EXPORT wasm_global_t *
wasm_extern_as_global(wasm_extern_t *external) {
  return static_cast<wasm_global_t *>(external->global());
}
WASMEDGE_CAPI_EXPORT wasm_table_t *
wasm_extern_as_table(wasm_extern_t *external) {
  return static_cast<wasm_table_t *>(external->table());
}
WASMEDGE_CAPI_EXPORT wasm_memory_t *
wasm_extern_as_memory(wasm_extern_t *external) {
  return static_cast<wasm_memory_t *>(external->memory());
}

WASMEDGE_CAPI_EXPORT const wasm_func_t *
wasm_extern_as_func_const(const wasm_extern_t *external) {
  return static_cast<const wasm_func_t *>(external->func());
}
WASMEDGE_CAPI_EXPORT const wasm_global_t *
wasm_extern_as_global_const(const wasm_extern_t *external) {
  return static_cast<const wasm_global_t *>(external->global());
}
WASMEDGE_CAPI_EXPORT const wasm_table_t *
wasm_extern_as_table_const(const wasm_extern_t *external) {
  return static_cast<const wasm_table_t *>(external->table());
}
WASMEDGE_CAPI_EXPORT const wasm_memory_t *
wasm_extern_as_memory_const(const wasm_extern_t *external) {
  return static_cast<const wasm_memory_t *>(external->memory());
}

// Module Instances
struct wasm_instance_t : wasm::Instance {};
WASM_DEFINE_REF(instance, wasm::Instance)

WASMEDGE_CAPI_EXPORT wasm_instance_t *
wasm_instance_new(wasm_store_t *store, const wasm_module_t *module,
                  const wasm_extern_vec_t *imports, wasm_trap_t **trap) {
  wasm::own<wasm::Trap> error;
  auto imports_ = reinterpret_cast<const wasm::vec<wasm::Extern *> *>(imports);
  auto instance = static_cast<wasm_instance_t *>(
      wasm::Instance::make(store, module, *imports_, &error).release());
  if (trap)
    *trap = static_cast<wasm_trap_t *>(error.release());
  return instance;
}

WASMEDGE_CAPI_EXPORT void wasm_instance_exports(const wasm_instance_t *instance,
                                                wasm_extern_vec_t *out) {
  *out = {instance->exports().size(),
          reinterpret_cast<wasm_extern_t **>(instance->exports().release())};
}

WASMEDGE_CAPI_EXPORT wasm_instance_t *
wasm_frame_instance(const wasm_frame_t *frame) {
  return static_cast<wasm_instance_t *>(
      static_cast<const wasm::Frame *>(frame)->instance());
}

#ifdef OWN
#undef OWN
#endif

#ifdef __cplusplus
} /// extern "C"
#endif

/// The followings are the C++ API implementation.

/// >>>>>>>> wasm::Config functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Config> wasm::Config::make() {
  return wasm::own<wasm::Config>(wasm_config_new());
}

void wasm::Config::destroy() { delete static_cast<wasm_config_t *>(this); }

/// <<<<<<<< wasm::Config functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Engine functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Engine> wasm::Engine::make(wasm::own<wasm::Config> &&config) {
  wasm_config_t *conf = static_cast<wasm_config_t *>(config.release());
  return wasm::own<wasm::Engine>(wasm_engine_new_with_config(conf));
}

void wasm::Engine::destroy() { delete static_cast<wasm_engine_t *>(this); }

/// <<<<<<<< wasm::Engine functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Store> wasm::Store::make(wasm::Engine *engine) {
  return wasm::own<wasm::Store>(
      wasm_store_new(static_cast<wasm_engine_t *>(engine)));
}

void wasm::Store::destroy() { delete static_cast<wasm_store_t *>(this); }

/// <<<<<<<< wasm::Store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::ValType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ValType::destroy() {
  /// Not to delete the struct because the ValTypes are global variables.
}

wasm::own<wasm::ValType> wasm::ValType::make(wasm::ValKind kind) {
  switch (kind) {
  case ValKind::I32:
    return own<ValType>(&wasm_valtype_i32);
  case ValKind::I64:
    return own<ValType>(&wasm_valtype_i64);
  case ValKind::F32:
    return own<ValType>(&wasm_valtype_f32);
  case ValKind::F64:
    return own<ValType>(&wasm_valtype_f64);
  case ValKind::ANYREF:
    return own<ValType>(&wasm_valtype_anyref);
  case ValKind::FUNCREF:
    return own<ValType>(&wasm_valtype_funcref);
  };
  return own<ValType>(nullptr);
}

wasm::own<wasm::ValType> wasm::ValType::copy() const { return make(kind()); }

wasm::ValKind wasm::ValType::kind() const {
  return (static_cast<const wasm_valtype_t *>(this))->mkind;
}

/// <<<<<<<< wasm::ValType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::FuncType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::FuncType::destroy() { delete static_cast<wasm_functype_t *>(this); }

wasm::own<wasm::FuncType>
wasm::FuncType::make(wasm::ownvec<wasm::ValType> &&params,
                     wasm::ownvec<wasm::ValType> &&results) {
  if (params && results) {
    return wasm::make_own(new (std::nothrow) wasm_functype_t(
        std::move(params), std::move(results)));
  }
  return nullptr;
}

wasm::own<wasm::FuncType> wasm::FuncType::copy() const {
  return make(params().deep_copy(), results().deep_copy());
}

const wasm::ownvec<wasm::ValType> &wasm::FuncType::params() const {
  return (static_cast<const wasm_functype_t *>(this))->mparams;
}

const wasm::ownvec<wasm::ValType> &wasm::FuncType::results() const {
  return (static_cast<const wasm_functype_t *>(this))->mresults;
}

/// <<<<<<<< wasm::FuncType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::GlobalType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::GlobalType::destroy() {
  delete static_cast<wasm_globaltype_t *>(this);
}

wasm::own<wasm::GlobalType>
wasm::GlobalType::make(wasm::own<wasm::ValType> &&content,
                       wasm::Mutability mutability) {
  if (content) {
    return wasm::make_own(
        new (std::nothrow) wasm_globaltype_t(std::move(content), mutability));
  }
  return nullptr;
}

wasm::own<wasm::GlobalType> wasm::GlobalType::copy() const {
  return make(content()->copy(), mutability());
}

const wasm::ValType *wasm::GlobalType::content() const {
  return (static_cast<const wasm_globaltype_t *>(this))->mcontent.get();
}

wasm::Mutability wasm::GlobalType::mutability() const {
  return (static_cast<const wasm_globaltype_t *>(this))->mmutability;
}

/// <<<<<<<< wasm::GlobalType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::TableType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::TableType::destroy() {
  delete static_cast<wasm_tabletype_t *>(this);
}

wasm::own<wasm::TableType>
wasm::TableType::make(wasm::own<wasm::ValType> &&element, wasm::Limits limits) {
  if (element) {
    return wasm::make_own(new (std::nothrow)
                              wasm_tabletype_t(std::move(element), limits));
  }
  return nullptr;
}

wasm::own<wasm::TableType> wasm::TableType::copy() const {
  return make(element()->copy(), limits());
}

const wasm::ValType *wasm::TableType::element() const {
  return (static_cast<const wasm_tabletype_t *>(this))->mvaltype.get();
}

const wasm::Limits &wasm::TableType::limits() const {
  return (static_cast<const wasm_tabletype_t *>(this))->mlimits;
}

/// <<<<<<<< wasm::TableType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::MemoryType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::MemoryType::destroy() {
  delete static_cast<wasm_memorytype_t *>(this);
}

wasm::own<wasm::MemoryType> wasm::MemoryType::make(wasm::Limits limits) {
  return wasm::make_own(new (std::nothrow) wasm_memorytype_t(limits));
}

wasm::own<wasm::MemoryType> wasm::MemoryType::copy() const {
  return make(limits());
}

const wasm::Limits &wasm::MemoryType::limits() const {
  return (static_cast<const wasm_memorytype_t *>(this))->mlimits;
}

/// <<<<<<<< wasm::MemoryType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::ExternType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ExternType::destroy() {
  switch (kind()) {
  case wasm::ExternKind::FUNC:
    delete static_cast<wasm_functype_t *>(this);
    break;
  case wasm::ExternKind::GLOBAL:
    delete static_cast<wasm_globaltype_t *>(this);
    break;
  case wasm::ExternKind::TABLE:
    delete static_cast<wasm_tabletype_t *>(this);
    break;
  case wasm::ExternKind::MEMORY:
    delete static_cast<wasm_memorytype_t *>(this);
    break;
  }
}

wasm::own<wasm::ExternType> wasm::ExternType::copy() const {
  switch (kind()) {
  case wasm::ExternKind::FUNC:
    return func()->copy();
  case wasm::ExternKind::GLOBAL:
    return global()->copy();
  case wasm::ExternKind::TABLE:
    return table()->copy();
  case wasm::ExternKind::MEMORY:
    return memory()->copy();
  }
  return nullptr;
}

wasm::ExternKind wasm::ExternType::kind() const {
  return static_cast<const wasm_externtype_t *>(this)->mkind;
}

WASM_EXTERNTYPE_CPP_CONV(func, wasm::FuncType, FUNC, )
WASM_EXTERNTYPE_CPP_CONV(func, wasm::FuncType, FUNC, const)
WASM_EXTERNTYPE_CPP_CONV(global, wasm::GlobalType, GLOBAL, )
WASM_EXTERNTYPE_CPP_CONV(global, wasm::GlobalType, GLOBAL, const)
WASM_EXTERNTYPE_CPP_CONV(table, wasm::TableType, TABLE, )
WASM_EXTERNTYPE_CPP_CONV(table, wasm::TableType, TABLE, const)
WASM_EXTERNTYPE_CPP_CONV(memory, wasm::MemoryType, MEMORY, )
WASM_EXTERNTYPE_CPP_CONV(memory, wasm::MemoryType, MEMORY, const)

/// <<<<<<<< wasm::ExternType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::ImportType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ImportType::destroy() {
  delete static_cast<wasm_importtype_t *>(this);
}

wasm::own<wasm::ImportType> wasm::ImportType::make(Name &&module, Name &&name,
                                                   own<ExternType> &&type) {
  if (module && name && type) {
    return wasm::make_own(new (std::nothrow) wasm_importtype_t(
        std::move(module), std::move(name), std::move(type)));
  }
  return nullptr;
}

wasm::own<wasm::ImportType> wasm::ImportType::copy() const {
  return make(module().copy(), name().copy(), type()->copy());
}

const wasm::Name &wasm::ImportType::module() const {
  return static_cast<const wasm_importtype_t *>(this)->mmodname;
}

const wasm::Name &wasm::ImportType::name() const {
  return static_cast<const wasm_importtype_t *>(this)->mname;
}

const wasm::ExternType *wasm::ImportType::type() const {
  return static_cast<const wasm_importtype_t *>(this)->mtype.get();
}

/// <<<<<<<< wasm::ImportType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::ExportType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ExportType::destroy() {
  delete static_cast<wasm_exporttype_t *>(this);
}

wasm::own<wasm::ExportType>
wasm::ExportType::make(wasm::Name &&name, wasm::own<wasm::ExternType> &&type) {
  if (name && type) {
    return wasm::make_own(
        new (std::nothrow) wasm_exporttype_t(std::move(name), std::move(type)));
  }
  return nullptr;
}

wasm::own<wasm::ExportType> wasm::ExportType::copy() const {
  return make(name().copy(), type()->copy());
}

const wasm::Name &wasm::ExportType::name() const {
  return static_cast<const wasm_exporttype_t *>(this)->mname;
}

const wasm::ExternType *wasm::ExportType::type() const {
  return static_cast<const wasm_exporttype_t *>(this)->mtype.get();
}

/// <<<<<<<< wasm::ExportType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::References functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Ref::destroy() {}
bool wasm::Ref::same(const wasm::Ref *that) const {
  UNUSED(that);
  return false;
}
WASMEDGE_CAPI_EXPORT void *wasm::Ref::get_host_info() const { return nullptr; }
WASMEDGE_CAPI_EXPORT void wasm::Ref::set_host_info(void *info,
                                                   void (*finalizer)(void *)) {
  UNUSED(info);
  UNUSED(finalizer);
}
wasm::own<wasm::Ref> wasm::Ref::copy() const { return wasm::own<wasm::Ref>(); }

/// <<<<<<<< wasm::References functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Frame functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Frame::destroy() {}
wasm::own<wasm::Frame> wasm::Frame::copy() const {
  return wasm::own<wasm::Frame>();
}
wasm::Instance *wasm::Frame::instance() const { return nullptr; }
uint32_t wasm::Frame::func_index() const { return 0; }
size_t wasm::Frame::func_offset() const { return 0; }
size_t wasm::Frame::module_offset() const { return 0; }

/// <<<<<<<< wasm::Frame functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Trap functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Trap::destroy() {}
wasm::own<wasm::Trap> wasm::Trap::make(wasm::Store *,
                                       const wasm::Message &msg) {
  UNUSED(msg);
  return wasm::own<wasm::Trap>();
}
wasm::Message wasm::Trap::message() const {
  return vec<byte_t>::make();
  //  return wasm::Message();
}
wasm::own<wasm::Frame> wasm::Trap::origin() const {
  return wasm::own<wasm::Frame>();
}
wasm::ownvec<wasm::Frame> wasm::Trap::trace() const {
  return ownvec<wasm::Frame>::make();
  //  return wasm::ownvec<wasm::Frame>(wasm::vec();
}
wasm::own<wasm::Trap> wasm::Trap::copy() const {
  return wasm::own<wasm::Trap>();
}

/// <<<<<<<< wasm::Trap functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Modules functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Module::destroy() {}
bool wasm::Module::validate(Store *, const vec<byte_t> &binary) {
  UNUSED(binary);
  return false;
}
wasm::own<wasm::Module> make(wasm::Store *, const wasm::vec<byte_t> &binary) {
  UNUSED(binary);
  return wasm::own<wasm::Module>();
}
wasm::own<wasm::Module> copy() { return wasm::own<wasm::Module>(); }
wasm::ownvec<wasm::ImportType> imports() {
  return wasm::ownvec<wasm::ImportType>::make_uninitialized();
}
wasm::ownvec<wasm::ExportType> exports() {
  return wasm::ownvec<wasm::ExportType>::make_uninitialized();
}
wasm::own<wasm::Shared<wasm::Module>> share() {
  return wasm::own<wasm::Shared<wasm::Module>>();
}
wasm::own<wasm::Module> obtain(wasm::Store *store,
                               const wasm::Shared<wasm::Module> *module) {
  UNUSED(store);
  UNUSED(module);
  return wasm::own<wasm::Module>();
}
wasm::vec<byte_t> wasm::Module::serialize() const {
  return vec<byte_t>::make_uninitialized();
}

wasm::own<wasm::Module> wasm::Module::deserialize(Store *store,
                                                  const vec<byte_t> &vec_byte) {
  UNUSED(store);
  UNUSED(vec_byte);
  return wasm::own<Module>();
}

/// <<<<<<<< wasm::Modules functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Shared functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Shared<wasm::Module>::destroy() {}

/// <<<<<<<< wasm::Shared functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Foreign functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Foreign::destroy() {}
wasm::own<wasm::Foreign> wasm::Foreign::make(Store *) {
  return wasm::own<wasm::Foreign>();
}
wasm::own<wasm::Foreign> wasm::Foreign::copy() const { return own<Foreign>(); }

/// <<<<<<<< wasm::Foreign functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Externals functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Extern::destroy() {}

wasm::own<wasm::Extern> wasm::Extern::copy() const { return own<Extern>(); }
wasm::ExternKind wasm::Extern::kind() const { return wasm::ExternKind::MEMORY; }
wasm::own<wasm::ExternType> wasm::Extern::type() const {
  return wasm::own<wasm::ExternType>();
}

wasm::Func *wasm::Extern::func() { return nullptr; }
wasm::Table *wasm::Extern::table() { return nullptr; }
wasm::Global *wasm::Extern::global() { return nullptr; }
wasm::Memory *wasm::Extern::memory() { return nullptr; }
const wasm::Func *wasm::Extern::func() const { return nullptr; }
const wasm::Global *wasm::Extern::global() const { return nullptr; }
const wasm::Table *wasm::Extern::table() const { return nullptr; }
const wasm::Memory *wasm::Extern::memory() const { return nullptr; }

/// <<<<<<<< wasm::Externals functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Func Instances functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Func::destroy() {}
wasm::own<wasm::Func> wasm::Func::make(Store *, const FuncType *,
                                       wasm::Func::callback) {
  return wasm::own<wasm::Func>();
}
wasm::own<wasm::Func> wasm::Func::make(Store *, const FuncType *,
                                       wasm::Func::callback_with_env, void *,
                                       void (*finalizer)(void *)) {
  UNUSED(finalizer);
  return wasm::own<wasm::Func>();
}
wasm::own<wasm::Func> wasm::Func::copy() const { return wasm::own<Func>(); }
wasm::own<wasm::FuncType> wasm::Func::type() const { return own<FuncType>(); }
size_t wasm::Func::param_arity() const { return 0; }
size_t wasm::Func::result_arity() const { return 0; }
wasm::own<wasm::Trap> wasm::Func::call(const vec<Val> &, vec<Val> &) const {
  return wasm::own<wasm::Trap>();
}
/// <<<<<<<< wasm::Func Instances functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Global Instances functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void wasm::Global::destroy() {}
wasm::own<wasm::Global> wasm::Global::make(Store *, const GlobalType *,
                                           const Val &) {
  return wasm::own<wasm::Global>();
}
wasm::own<wasm::Global> wasm::Global::copy() const { return own<Global>(); }
wasm::own<wasm::GlobalType> wasm::Global::type() const {
  return own<GlobalType>();
}
wasm::Val wasm::Global::get() const { return Val(); }
void wasm::Global::set(const Val &) {}

/// <<<<<<<< wasm::Global Instances functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Table Instances functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void wasm::Table::destroy() {}
wasm::own<wasm::Table> wasm::Table::make(Store *, const TableType *,
                                         const Ref *init) {
  UNUSED(init);
  return wasm::own<wasm::Table>();
}
wasm::own<wasm::Table> wasm::Table::copy() const { return own<Table>(); }
wasm::own<wasm::TableType> wasm::Table::type() const {
  return own<TableType>();
}
wasm::own<wasm::Ref> wasm::Table::get(Table::size_t index) const {
  UNUSED(index);
  return wasm::own<wasm::Ref>();
}
bool wasm::Table::set(Table::size_t index, const Ref *) {
  UNUSED(index);
  return false;
}
wasm::Table::size_t wasm::Table::size() const { return 0; }
bool wasm::Table::grow(Table::size_t delta, const Ref *init) {
  UNUSED(delta);
  UNUSED(init);
  return false;
}

/// <<<<<<<< wasm::Table Instances functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Memory Instances functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Memory::destroy() {}
wasm::own<wasm::Memory> wasm::Memory::make(Store *, const MemoryType *) {
  return wasm::own<wasm::Memory>();
}
wasm::own<wasm::Memory> wasm::Memory::copy() const { return own<Memory>(); }
wasm::own<wasm::MemoryType> wasm::Memory::type() const {
  return own<MemoryType>();
}
byte_t *wasm::Memory::data() const { return nullptr; }
size_t wasm::Memory::data_size() const { return 0; }
wasm::Memory::pages_t wasm::Memory::size() const { return 0; }
bool wasm::Memory::grow(Memory::pages_t delta) {
  UNUSED(delta);
  return false;
}

/// <<<<<<<< wasm::Memory Instances functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// >>>>>>>> wasm::Module Instances functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Instance::destroy() {}
wasm::own<wasm::Instance> wasm::Instance::make(Store *, const Module *,
                                               const vec<Extern *> &,
                                               own<Trap> *) {
  return wasm::own<wasm::Instance>();
}
wasm::own<wasm::Instance> wasm::Instance::copy() const {
  return own<Instance>();
}
wasm::ownvec<wasm::Extern> wasm::Instance::exports() const {
  return wasm::ownvec<Extern>::make_uninitialized();
}
/// <<<<<<<< wasm::Module  Instances functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
