// SPDX-License-Identifier: Apache-2.0
#include <algorithm>

#include "common/configure.h"
#include "common/errcode.h"
#include "common/log.h"
#include "common/span.h"
#include "common/value.h"

#include "aot/compiler.h"
#include "ast/module.h"
#include "interpreter/interpreter.h"
#include "loader/loader.h"
#include "runtime/storemgr.h"
#include "validator/validator.h"

#include "api/wasmedge.h"
#include "wasm/wasm.h"
#include "wasm/wasm.hh"

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
  wasm_engine_t() noexcept : conf(), interp(conf, nullptr) {}
  wasm_engine_t(const wasm_config_t *c) noexcept
      : conf(c->conf), interp(c->conf, nullptr) {}
  ~wasm_engine_t() noexcept = default;
  WasmEdge::Configure conf;
  WasmEdge::Interpreter::Interpreter interp;
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
      wasm::vec<Name>::adopt(vec->size, vec->data);                            \
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
