// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <unordered_map>

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

namespace {

// WasmEdge::ValType to wasm::ValType conversion.
wasm::ValKind conv_valtype_from_wasmedge(WasmEdge::ValType vt) {
  switch (vt) {
  case WasmEdge::ValType::I32:
    return wasm::ValKind::I32;
  case WasmEdge::ValType::I64:
    return wasm::ValKind::I64;
  case WasmEdge::ValType::F32:
    return wasm::ValKind::F32;
  case WasmEdge::ValType::F64:
    return wasm::ValKind::F64;
  case WasmEdge::ValType::FuncRef:
    return wasm::ValKind::FUNCREF;
  case WasmEdge::ValType::ExternRef:
    return wasm::ValKind::ANYREF;
  case WasmEdge::ValType::V128:
    // v128 is not supported in wasm C API yet.
  default:
    assumingUnreachable();
  }
}

// WasmEdge::RefType to wasm::ValType conversion.
wasm::ValKind conv_reftype_from_wasmedge(WasmEdge::RefType vt) {
  switch (vt) {
  case WasmEdge::RefType::FuncRef:
    return wasm::ValKind::FUNCREF;
  case WasmEdge::RefType::ExternRef:
    return wasm::ValKind::ANYREF;
    // v128 is not supported in wasm C API yet.
  default:
    assumingUnreachable();
  }
}

// WasmEdge::ValMut to wasm::Mutability conversion.
wasm::Mutability conv_valmut_from_wasmedge(WasmEdge::ValMut mut) {
  switch (mut) {
  case WasmEdge::ValMut::Const:
    return wasm::Mutability::CONST;
  case WasmEdge::ValMut::Var:
    return wasm::Mutability::VAR;
  default:
    assumingUnreachable();
  }
}

// wasm::ValType to WasmEdge::ValType conversion.
WasmEdge::ValType conv_valtype_to_wasmedge(wasm::ValKind vt) {
  switch (vt) {
  case wasm::ValKind::I32:
    return WasmEdge::ValType::I32;
  case wasm::ValKind::I64:
    return WasmEdge::ValType::I64;
  case wasm::ValKind::F32:
    return WasmEdge::ValType::F32;
  case wasm::ValKind::F64:
    return WasmEdge::ValType::F64;
  case wasm::ValKind::FUNCREF:
    return WasmEdge::ValType::FuncRef;
  case wasm::ValKind::ANYREF:
    return WasmEdge::ValType::ExternRef;
  default:
    assumingUnreachable();
  }
}

// wasm::ValType to WasmEdge::RefType conversion.
WasmEdge::RefType conv_reftype_to_wasmedge(wasm::ValKind vt) {
  switch (vt) {
  case wasm::ValKind::FUNCREF:
    return WasmEdge::RefType::FuncRef;
  case wasm::ValKind::ANYREF:
    return WasmEdge::RefType::ExternRef;
  default:
    assumingUnreachable();
  }
}

// wasm::Mutability to WasmEdge::ValMut conversion.
WasmEdge::ValMut conv_valmut_to_wasmedge(wasm::Mutability mut) {
  switch (mut) {
  case wasm::Mutability::CONST:
    return WasmEdge::ValMut::Const;
  case wasm::Mutability::VAR:
    return WasmEdge::ValMut::Var;
  default:
    assumingUnreachable();
  }
}

// Make wasm::Limits from WasmEdge::AST::Limit.
wasm::Limits make_limits_from_wasmedge(const WasmEdge::AST::Limit &lm) {
  if (lm.hasMax()) {
    return wasm::Limits(lm.getMin(), lm.getMax());
  } else {
    return wasm::Limits(lm.getMin());
  }
}

// Make wasm::own<wasm::FuncType> from WasmEdge::AST::FunctionType.
wasm::own<wasm::FuncType>
make_functype_from_wasmedge(const WasmEdge::AST::FunctionType &ft) {
  auto p = wasm::ownvec<wasm::ValType>::make_uninitialized(
      ft.getParamTypes().size());
  for (size_t i = 0; i < p.size(); ++i) {
    p[i] =
        wasm::ValType::make(conv_valtype_from_wasmedge(ft.getParamTypes()[i]));
  }
  auto r = wasm::ownvec<wasm::ValType>::make_uninitialized(
      ft.getReturnTypes().size());
  for (size_t i = 0; i < r.size(); ++i) {
    r[i] =
        wasm::ValType::make(conv_valtype_from_wasmedge(ft.getReturnTypes()[i]));
  }
  return wasm::FuncType::make(std::move(p), std::move(r));
}

// Make WasmEdge::AST::FunctionType from wasm::FuncType.
WasmEdge::AST::FunctionType make_functype_from_wasm(const wasm::FuncType &ft) {
  std::vector<WasmEdge::ValType> p(ft.params().size()), r(ft.results().size());
  for (size_t i = 0; i < p.size(); ++i) {
    p[i] = conv_valtype_to_wasmedge(ft.params()[i]->kind());
  }
  for (size_t i = 0; i < r.size(); ++i) {
    r[i] = conv_valtype_to_wasmedge(ft.results()[i]->kind());
  }
  return WasmEdge::AST::FunctionType(p, r);
}

// Make wasm::own<wasm::GlobalType> from WasmEdge::AST::GlobalType.
wasm::own<wasm::GlobalType>
make_globaltype_from_wasmedge(const WasmEdge::AST::GlobalType &gt) {
  return wasm::GlobalType::make(
      wasm::ValType::make(conv_valtype_from_wasmedge(gt.getValType())),
      conv_valmut_from_wasmedge(gt.getValMut()));
}

// Make wasm::own<wasm::TableType> from WasmEdge::AST::TableType.
wasm::own<wasm::TableType>
make_tabletype_from_wasmedge(const WasmEdge::AST::TableType &tt) {
  return wasm::TableType::make(
      wasm::ValType::make(conv_reftype_from_wasmedge(tt.getRefType())),
      make_limits_from_wasmedge(tt.getLimit()));
}

// Make wasm::own<wasm::MemoryType> from WasmEdge::AST::MemoryType.
wasm::own<wasm::MemoryType>
make_memorytype_from_wasmedge(const WasmEdge::AST::MemoryType &mt) {
  return wasm::MemoryType::make(make_limits_from_wasmedge(mt.getLimit()));
}

// wasm_allocmgr_t: the allocation manager for the runtime objects.
struct wasm_allocmgr_t {
  template <typename T>
  void link(
      T *target, void (*destroyer)(void *) = [](void *target) {
        delete static_cast<T *>(target);
      }) {
    auto it = refcnts.find(static_cast<void *>(target));
    if (it != refcnts.end()) {
      refcnts.insert(
          {static_cast<void *>(target),
           std::make_pair<size_t, std::function<void(void *)>>(1, destroyer)});
    } else {
      it->second.first += 1;
    }
  }

  template <typename T> void unlink(T *target) {
    auto it = refcnts.find(static_cast<void *>(target));
    if (it != refcnts.end()) {
      it->second.first -= 1;
      if (it->second.first == 0) {
        if (target->finalizer) {
          target->finalizer(target->info);
        }
        it->second.second(target);
        refcnts.erase(it);
      }
    }
  }

  ~wasm_allocmgr_t();

private:
  std::unordered_map<void *, std::pair<size_t, std::function<void(void *)>>>
      refcnts;
  std::unordered_map<const WasmEdge::Runtime::Instance::FunctionInstance *,
                     void *>
      funcmap;
};

// Value conversion forward declaration.
inline wasm_val_t conv_cval_from_wasmedge(const WasmEdge::ValVariant &,
                                          WasmEdge::ValType);
inline wasm::Val conv_cppval_from_wasmedge(const WasmEdge::ValVariant &,
                                           WasmEdge::ValType);
inline WasmEdge::ValVariant conv_cval_to_wasmedge(const wasm_val_t &);
inline WasmEdge::ValVariant conv_cppval_to_wasmedge(const wasm::Val &,
                                                    wasm::ValKind);
inline wasm::Val conv_val_to_cpp(const wasm_val_t &);
inline wasm_val_t conv_val_to_c(const wasm::Val &, wasm::ValKind);

// wasm_hostfunc_t: the wrapper to host function in wasm C/C++ API.
class wasm_hostfunc_t : public WasmEdge::Runtime::HostFunctionBase {
public:
  wasm_hostfunc_t(const wasm::FuncType &ft, wasm_func_callback_t cb) noexcept
      : WasmEdge::Runtime::HostFunctionBase(0), type(api_type_enum::C),
        callback({.c = cb}), env(nullptr), finalizer(nullptr) {
    FuncType = make_functype_from_wasm(ft);
  }
  wasm_hostfunc_t(const wasm::FuncType &ft, wasm_func_callback_with_env_t cb,
                  void *e, void (*fin)(void *)) noexcept
      : WasmEdge::Runtime::HostFunctionBase(0), type(api_type_enum::C_ENV),
        callback({.c_env = cb}), env(e), finalizer(fin) {
    FuncType = make_functype_from_wasm(ft);
  }
  wasm_hostfunc_t(const wasm::FuncType &ft, wasm::Func::callback cb) noexcept
      : WasmEdge::Runtime::HostFunctionBase(0), type(api_type_enum::CPP),
        callback({.cpp = cb}), env(nullptr), finalizer(nullptr) {
    FuncType = make_functype_from_wasm(ft);
  }
  wasm_hostfunc_t(const wasm::FuncType &ft, wasm::Func::callback_with_env cb,
                  void *e, void (*fin)(void *)) noexcept
      : WasmEdge::Runtime::HostFunctionBase(0), type(api_type_enum::CPP_ENV),
        callback({.cpp_env = cb}), env(e), finalizer(fin) {
    FuncType = make_functype_from_wasm(ft);
  }
  ~wasm_hostfunc_t() noexcept override {
    if (finalizer) {
      finalizer(env);
    }
  };

  WasmEdge::Expect<void>
  run(const WasmEdge::Runtime::CallingFrame &,
      WasmEdge::Span<const WasmEdge::ValVariant> Args,
      WasmEdge::Span<WasmEdge::ValVariant> Rets) override {
    auto &ptypes = FuncType.getParamTypes();
    auto &rtypes = FuncType.getReturnTypes();

    if (type == api_type_enum::C || type == api_type_enum::C_ENV) {
      // Prepare the arguments.
      std::vector<wasm_val_t> pdata(ptypes.size());
      std::vector<wasm_val_t> rdata(rtypes.size());
      wasm_val_vec_t params{.size = ptypes.size(), .data = pdata.data()};
      wasm_val_vec_t returns{.size = rtypes.size(), .data = rdata.data()};
      for (size_t i = 0; i < Args.size(); ++i) {
        params.data[i] = conv_cval_from_wasmedge(Args[i], ptypes[i]);
      }

      // Invokation.
      wasm_trap_t *trap = nullptr;
      if (callback.c) {
        if (type == api_type_enum::C) {
          trap = callback.c(&params, &returns);
        } else {
          trap = callback.c_env(env, &params, &returns);
        }
      }

      // Get the results.
      if (trap) {
        wasm_trap_delete(trap);
        return Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
      }
      for (size_t i = 0; i < Rets.size(); ++i) {
        Rets[i] = conv_cval_to_wasmedge(returns.data[i]);
      }
    } else {
      // Prepare the arguments.
      auto params = wasm::vec<wasm::Val>::make_uninitialized(ptypes.size());
      auto returns = wasm::vec<wasm::Val>::make_uninitialized(rtypes.size());
      for (size_t i = 0; i < Args.size(); ++i) {
        params[i] = conv_cppval_from_wasmedge(Args[i], ptypes[i]);
      }

      // Invokation.
      wasm::own<wasm::Trap> trap;
      if (callback.cpp) {
        if (type == api_type_enum::CPP) {
          trap = callback.cpp(params, returns);
        } else {
          trap = callback.cpp_env(env, params, returns);
        }
      }

      // Get the results.
      if (trap) {
        return Unexpect(WasmEdge::ErrCode::Value::HostFuncError);
      }
      for (size_t i = 0; i < Rets.size(); ++i) {
        Rets[i] = conv_cppval_to_wasmedge(
            returns[i], conv_valtype_from_wasmedge(rtypes[i]));
      }
    }
    return {};
  }

private:
  enum class api_type_enum { C, CPP, C_ENV, CPP_ENV };
  api_type_enum type;
  union {
    wasm_func_callback_t c;
    wasm::Func::callback cpp;
    wasm_func_callback_with_env_t c_env;
    wasm::Func::callback_with_env cpp_env;
  } callback;
  void *env;
  void (*finalizer)(void *);
};

// wasm_category_val implementation.
template <class C> struct wasm_externkind_val;
template <> struct wasm_externkind_val<wasm::FuncType> {
  static const auto value = wasm::ExternKind::FUNC;
};
template <> struct wasm_externkind_val<wasm::GlobalType> {
  static const auto value = wasm::ExternKind::GLOBAL;
};
template <> struct wasm_externkind_val<wasm::TableType> {
  static const auto value = wasm::ExternKind::TABLE;
};
template <> struct wasm_externkind_val<wasm::MemoryType> {
  static const auto value = wasm::ExternKind::MEMORY;
};
template <> struct wasm_externkind_val<wasm::Func> {
  static const auto value = wasm::ExternKind::FUNC;
};
template <> struct wasm_externkind_val<wasm::Global> {
  static const auto value = wasm::ExternKind::GLOBAL;
};
template <> struct wasm_externkind_val<wasm::Table> {
  static const auto value = wasm::ExternKind::TABLE;
};
template <> struct wasm_externkind_val<wasm::Memory> {
  static const auto value = wasm::ExternKind::MEMORY;
};

// wasm_category_enum for the runtime objects.
enum class wasm_category_enum : uint8_t {
  TRAP,
  FOREIGN,
  MODULE,
  FUNC,
  GLOBAL,
  TABLE,
  MEMORY,
  INSTANCE
};

// wasm_category_val implementation.
template <class C> struct wasm_category_val;
template <> struct wasm_category_val<wasm_trap_t> {
  static const auto value = wasm_category_enum::TRAP;
};
template <> struct wasm_category_val<wasm_foreign_t> {
  static const auto value = wasm_category_enum::FOREIGN;
};
template <> struct wasm_category_val<wasm_module_t> {
  static const auto value = wasm_category_enum::MODULE;
};
template <> struct wasm_category_val<wasm_func_t> {
  static const auto value = wasm_category_enum::FUNC;
};
template <> struct wasm_category_val<wasm_global_t> {
  static const auto value = wasm_category_enum::GLOBAL;
};
template <> struct wasm_category_val<wasm_table_t> {
  static const auto value = wasm_category_enum::TABLE;
};
template <> struct wasm_category_val<wasm_memory_t> {
  static const auto value = wasm_category_enum::MEMORY;
};
/*
template <> struct wasm_category_val<wasm_instance_t> {
  static const auto value = wasm_category_enum::INSTANCE;
};
*/

} // namespace

// wasm_config_t implementation.
struct wasm_config_t : public wasm::Config {
  wasm_config_t() noexcept = default;
  WasmEdge::Configure conf;
};

// wasm_engine_t implementation.
struct wasm_engine_t : public wasm::Engine {
  wasm_engine_t() noexcept
      : mconf(new (std::nothrow) wasm_config_t),
        exec(static_cast<wasm_config_t *>(mconf.get())->conf, nullptr) {}
  wasm_engine_t(wasm::own<wasm::Config> &&c) noexcept
      : mconf(std::move(c)),
        exec(static_cast<wasm_config_t *>(mconf.get())->conf, nullptr) {}
  wasm::own<wasm::Config> mconf;
  WasmEdge::Executor::Executor exec;
};

// wasm_store_t implementation.
struct wasm_store_t : public wasm::Store {
  wasm_store_t(wasm::Engine *e) noexcept
      : engine(static_cast<wasm_engine_t *>(e)),
        load(static_cast<wasm_config_t *>(engine->mconf.get())->conf),
        valid(static_cast<wasm_config_t *>(engine->mconf.get())->conf),
        store() {}
  wasm_engine_t *engine;
  wasm_allocmgr_t allocmgr;
  WasmEdge::Loader::Loader load;
  WasmEdge::Validator::Validator valid;
  WasmEdge::Runtime::StoreManager store;
};

// wasm_valtype_t implementation.
struct wasm_valtype_t : public wasm::ValType {
  wasm_valtype_t(wasm::ValKind k) noexcept : mkind(k) {}
  wasm::ValKind mkind;
};

// wasm_externtype_base_t implementation.
template <class C> struct wasm_externtype_base_t : public C {
  wasm_externtype_base_t() noexcept : mkind(wasm_externkind_val<C>::value) {}
  wasm::ExternKind mkind;
};

// wasm_externtype_t implementation.
struct wasm_externtype_t : wasm_externtype_base_t<wasm::ExternType> {
  wasm_externtype_t() = delete;
};

// wasm_functype_t implementation.
struct wasm_functype_t : public wasm_externtype_base_t<wasm::FuncType> {
  wasm_functype_t(wasm::ownvec<wasm::ValType> &&vp,
                  wasm::ownvec<wasm::ValType> &&vr) noexcept
      : mparams(std::move(vp)), mresults(std::move(vr)) {}
  wasm::ownvec<wasm::ValType> mparams, mresults;
};

// wasm_globaltype_t implementation.
struct wasm_globaltype_t : public wasm_externtype_base_t<wasm::GlobalType> {
  wasm_globaltype_t(wasm::own<wasm::ValType> &&vt,
                    wasm::Mutability mut) noexcept
      : mcontent(std::move(vt)), mmutability(mut) {}
  wasm::own<wasm::ValType> mcontent;
  wasm::Mutability mmutability;
};

// wasm_tabletype_t implementation.
struct wasm_tabletype_t : public wasm_externtype_base_t<wasm::TableType> {
  wasm_tabletype_t(wasm::own<wasm::ValType> &&vt, wasm::Limits lim) noexcept
      : mvaltype(std::move(vt)), mlimits(lim) {}
  wasm::own<wasm::ValType> mvaltype;
  wasm::Limits mlimits;
};

// wasm_memorytype_t implementation.
struct wasm_memorytype_t : public wasm_externtype_base_t<wasm::MemoryType> {
  wasm_memorytype_t(wasm::Limits lim) noexcept : mlimits(lim) {}
  ~wasm_memorytype_t() noexcept = default;
  wasm::Limits mlimits;
};

// wasm_importtype_t implementation.
struct wasm_importtype_t : public wasm::ImportType {
  wasm_importtype_t(wasm::Name &&mod, wasm::Name &&n,
                    wasm::own<wasm::ExternType> &&ext) noexcept
      : mmodname(std::move(mod)), mname(std::move(n)), mtype(std::move(ext)) {}
  wasm::Name mmodname, mname;
  wasm::own<wasm::ExternType> mtype;
};

// wasm_exporttype_t implementation.
struct wasm_exporttype_t : public wasm::ExportType {
  wasm_exporttype_t(wasm::Name &&n, wasm::own<wasm::ExternType> &&ext) noexcept
      : mname(std::move(n)), mtype(std::move(ext)) {}
  wasm::Name mname;
  wasm::own<wasm::ExternType> mtype;
};

// wasm_ref_base_t implementation.
template <class C> struct wasm_ref_base_t {
  wasm_ref_base_t(wasm::Store *s) noexcept
      : store(static_cast<wasm_store_t *>(s)),
        category(wasm_category_val<C>::value), inner(this), info(nullptr),
        finalizer(nullptr) {
    increase_ref();
  }
  wasm_ref_base_t(wasm::Store *s, const wasm_ref_base_t &ref) noexcept
      : store(static_cast<wasm_store_t *>(s)), category(ref.category),
        inner(ref.inner), info(ref.info), finalizer(ref.finalizer) {
    increase_ref();
  }
  virtual ~wasm_ref_base_t() noexcept {
    store->allocmgr.unlink<C>(static_cast<C *>(inner));
  }
  void increase_ref() const noexcept {
    store->allocmgr.link<C>(static_cast<C *>(inner));
  }
  wasm::own<C> copy_ref() const noexcept {
    increase_ref();
    return wasm::make_own<C>(static_cast<C *>(inner));
  }

  wasm_store_t *store;
  wasm_category_enum category;
  void *inner;
  void *info;
  void (*finalizer)(void *);
};

// wasm_ref_t implementation.
struct wasm_ref_t : public wasm_ref_base_t<wasm_ref_t>, wasm::Ref {
  wasm_ref_t() = delete;
};

// wasm_frame_t implementation.
// TODO: the tracing frame is not supported yet.
struct wasm_frame_t : public wasm::Frame {
  wasm_frame_t() noexcept : funcidx(0), funcoff(0), moduleoff(0) {}
  wasm_frame_t(const wasm_frame_t &frame) noexcept
      : funcidx(frame.funcidx), funcoff(frame.funcoff),
        moduleoff(frame.moduleoff) {}
  uint32_t funcidx;
  size_t funcoff;
  size_t moduleoff;
};

// wasm_trap_t implementation.
// TODO: the trap only supports error message in current.
struct wasm_trap_t : public wasm::Trap, wasm_ref_base_t<wasm_trap_t> {
  wasm_trap_t(wasm::Store *s, wasm::Message &&m) noexcept
      : wasm_ref_base_t<wasm_trap_t>(s), mmsg(std::move(m)) {}
  wasm::Message mmsg;
};

// wasm_foreign_t implementation.
// TODO: the foreign object is not supported yet.
struct wasm_foreign_t : public wasm::Foreign, wasm_ref_base_t<wasm_foreign_t> {
  wasm_foreign_t(wasm::Store *s) noexcept
      : wasm_ref_base_t<wasm_foreign_t>(s) {}
};

// wasm_module_t implementation.
struct wasm_module_t : public wasm::Module, wasm_ref_base_t<wasm_module_t> {
  wasm_module_t(wasm::Store *s, std::unique_ptr<WasmEdge::AST::Module> &&mod,
                wasm::vec<byte_t> &&bin) noexcept
      : wasm_ref_base_t<wasm_module_t>(s), ast(std::move(mod)),
        binary(std::move(bin)) {}
  std::unique_ptr<WasmEdge::AST::Module> ast;
  wasm::vec<byte_t> binary;
};

// wasm_shared_module_t implementation.
struct wasm_shared_module_t : public wasm::Shared<wasm::Module> {
  // Currently, the shared object is only used in modules.
  // Therefore we can use the serialize/deserialize mechanism to share and
  // obtain the object here. If there are other types of shared objects in the
  // future, the implementations should be updated.
  wasm_shared_module_t(wasm::vec<byte_t> &&bin) noexcept
      : binary(std::move(bin)) {}
  wasm::vec<byte_t> binary;
};

// wasm_extern_t implementation.
struct wasm_extern_t : public wasm_externtype_base_t<wasm::Extern>,
                       wasm_ref_base_t<wasm_extern_t> {
  wasm_extern_t() = delete;
};

// wasm_func_t implementation.
struct wasm_func_t : public wasm_externtype_base_t<wasm::Func>,
                     wasm_ref_base_t<wasm_func_t> {
  wasm_func_t(wasm::Store *s, const wasm::FuncType &ft,
              wasm_func_callback_t cb) noexcept
      : wasm_ref_base_t<wasm_func_t>(s), owned(true) {
    inst = new WasmEdge::Runtime::Instance::FunctionInstance(
        nullptr, std::make_unique<wasm_hostfunc_t>(ft, cb));
  }
  wasm_func_t(wasm::Store *s, const wasm::FuncType &ft,
              wasm_func_callback_with_env_t cb, void *env,
              void (*fin)(void *)) noexcept
      : wasm_ref_base_t<wasm_func_t>(s), owned(true) {
    inst = new WasmEdge::Runtime::Instance::FunctionInstance(
        nullptr, std::make_unique<wasm_hostfunc_t>(ft, cb, env, fin));
  }
  wasm_func_t(wasm::Store *s, const wasm::FuncType &ft,
              wasm::Func::callback cb) noexcept
      : wasm_ref_base_t<wasm_func_t>(s), owned(true) {
    inst = new WasmEdge::Runtime::Instance::FunctionInstance(
        nullptr, std::make_unique<wasm_hostfunc_t>(ft, cb));
  }
  wasm_func_t(wasm::Store *s, const wasm::FuncType &ft,
              wasm::Func::callback_with_env cb, void *env,
              void (*fin)(void *)) noexcept
      : wasm_ref_base_t<wasm_func_t>(s), owned(true) {
    inst = new WasmEdge::Runtime::Instance::FunctionInstance(
        nullptr, std::make_unique<wasm_hostfunc_t>(ft, cb, env, fin));
  }
  ~wasm_func_t() {
    if (owned && inst) {
      delete inst;
    }
  }
  bool owned;
  WasmEdge::Runtime::Instance::FunctionInstance *inst;
};

// wasm_global_t implementation.
struct wasm_global_t : public wasm_externtype_base_t<wasm::Global>,
                       wasm_ref_base_t<wasm_global_t> {
  wasm_global_t(wasm::Store *s, const wasm::GlobalType &gt,
                const wasm::Val &v) noexcept
      : wasm_ref_base_t<wasm_global_t>(s), keepval(v.copy()),
        inst(WasmEdge::AST::GlobalType(
                 conv_valtype_to_wasmedge(gt.content()->kind()),
                 conv_valmut_to_wasmedge(gt.mutability())),
             conv_cppval_to_wasmedge(v, gt.content()->kind())) {}
  // Copy the init val to keep the reference if the valtype is a ref.
  wasm::Val keepval;
  WasmEdge::Runtime::Instance::GlobalInstance inst;
};

// wasm_table_t implementation.
struct wasm_table_t : public wasm_externtype_base_t<wasm::Table>,
                      wasm_ref_base_t<wasm_table_t> {
  wasm_table_t(wasm::Store *s, const wasm::TableType &tt,
               const wasm::Ref *r) noexcept
      : wasm_ref_base_t<wasm_table_t>(s),
        inst(WasmEdge::AST::TableType(
            conv_reftype_to_wasmedge(tt.element()->kind()), tt.limits().min,
            tt.limits().max)) {
    // The wasm::Ref set into the table instance will not add the ref count.
    if (inst.getTableType().getRefType() == WasmEdge::RefType::FuncRef) {
      if (r && static_cast<const wasm_ref_t *>(r)->category ==
                   wasm_category_enum::FUNC) {
        inst.fillRefs(
            WasmEdge::FuncRef(static_cast<const wasm_func_t *>(r)->inst), 0,
            inst.getSize());
      } else {
        inst.fillRefs(WasmEdge::FuncRef(), 0, inst.getSize());
      }
    } else {
      inst.fillRefs(WasmEdge::ExternRef(const_cast<wasm::Ref *>(r)), 0,
                    inst.getSize());
    }
  }
  WasmEdge::Runtime::Instance::TableInstance inst;
};

// wasm_memory_t implementation.
struct wasm_memory_t : public wasm_externtype_base_t<wasm::Memory>,
                       wasm_ref_base_t<wasm_memory_t> {
  wasm_memory_t(wasm::Store *s, const wasm::MemoryType &mt) noexcept
      : wasm_ref_base_t<wasm_memory_t>(s),
        inst(WasmEdge::AST::MemoryType(mt.limits().min, mt.limits().max)) {}
  WasmEdge::Runtime::Instance::MemoryInstance inst;
};

// wasm_instance_t implementation.
struct wasm_instance_t : public wasm::Instance,
                         wasm_ref_base_t<wasm_instance_t> {};

namespace {

// Global variables to prevent from new/delete.
wasm_valtype_t wasm_valtype_i32(wasm::ValKind::I32);
wasm_valtype_t wasm_valtype_i64(wasm::ValKind::I64);
wasm_valtype_t wasm_valtype_f32(wasm::ValKind::F32);
wasm_valtype_t wasm_valtype_f64(wasm::ValKind::F64);
wasm_valtype_t wasm_valtype_anyref(wasm::ValKind::ANYREF);
wasm_valtype_t wasm_valtype_funcref(wasm::ValKind::FUNCREF);

// Helper macro for the C APIs to get attributtes.
#define WASM_DECLARE_C_GET_ATTR(name, target, attr, cast_type, def_val,        \
                                is_ref, is_const)                              \
  WASMEDGE_CAPI_EXPORT is_const target wasm_##name##_##attr(                   \
      const wasm_##name##_t *ptr) {                                            \
    if (ptr) {                                                                 \
      return cast_type##_cast<is_const target>(is_ref(ptr->attr()));           \
    }                                                                          \
    return def_val;                                                            \
  }

// Helper macro for the WASM_DECLARE_OWN implementation.
#define WASM_DECLARE_OWN_IMPL(name, Name)                                      \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_delete(OWN wasm_##name##_t *obj) {   \
    wasm::own<Name> ptr(obj);                                                  \
  }

// Helper macro for the base of WASM_DECLARE_VEC implementation.
#define WASM_DECLARE_VEC_BASE_IMPL(name, Name, ownvec, vec, ptr)               \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_uninitialized(               \
      OWN wasm_##name##_vec_t *out, size_t size) {                             \
    if (out) {                                                                 \
      if (size < UINT32_MAX) {                                                 \
        auto v = ownvec<Name>::make_uninitialized(size);                       \
        out->size = v.size();                                                  \
        out->data = reinterpret_cast<wasm_##name##_t ptr *>(v.release());      \
      } else {                                                                 \
        out->size = 0;                                                         \
        out->data = nullptr;                                                   \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new_empty(                       \
      OWN wasm_##name##_vec_t *out) {                                          \
    wasm_##name##_vec_new_uninitialized(out, 0);                               \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_delete(                          \
      OWN wasm_##name##_vec_t *v) {                                            \
    if (v) {                                                                   \
      ownvec<Name>::adopt(                                                     \
          v->size, reinterpret_cast<ownvec<Name>::elem_type *>(v->data));      \
      v->size = 0;                                                             \
      v->data = nullptr;                                                       \
    }                                                                          \
  }

// Helper macro for the parts of WASM_DECLARE_VEC implementation.
#define WASM_DECLARE_VEC_SCALAR_IMPL(name, Name)                               \
  WASM_DECLARE_VEC_BASE_IMPL(name, Name, wasm::vec, wasm::vec, )               \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new(                             \
      OWN wasm_##name##_vec_t *out, size_t size,                               \
      OWN wasm_##name##_t const vals[]) {                                      \
    if (out) {                                                                 \
      *out = {0, nullptr};                                                     \
      if (size < UINT32_MAX) {                                                 \
        auto v = wasm::vec<Name>::make_uninitialized(size);                    \
        if (v.size() > 0) {                                                    \
          std::copy_n(vals, size, v.get());                                    \
        }                                                                      \
        *out = {v.size(), reinterpret_cast<wasm_##name##_t *>(v.release())};   \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_copy(                            \
      OWN wasm_##name##_vec_t *out, const wasm_##name##_vec_t *in) {           \
    if (out) {                                                                 \
      *out = {0, nullptr};                                                     \
    }                                                                          \
    if (in && out) {                                                           \
      wasm_##name##_vec_new(out, in->size, in->data);                          \
    }                                                                          \
  }

// Helper macro for the parts of WASM_DECLARE_VEC implementation.
#define WASM_DECLARE_VEC_PTR_IMPL(name, Name)                                  \
  WASM_DECLARE_VEC_BASE_IMPL(name, Name, wasm::ownvec, wasm::vec, *)           \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_new(                             \
      OWN wasm_##name##_vec_t *out, size_t size,                               \
      OWN wasm_##name##_t *const data[]) {                                     \
    if (out) {                                                                 \
      *out = {0, nullptr};                                                     \
      if (size < UINT32_MAX) {                                                 \
        auto v = wasm::ownvec<Name>::make_uninitialized(size);                 \
        for (size_t i = 0; i < v.size(); ++i) {                                \
          v[i] = wasm::make_own(data[i]);                                      \
        }                                                                      \
        *out = {v.size(), reinterpret_cast<wasm_##name##_t **>(v.release())};  \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT void wasm_##name##_vec_copy(                            \
      OWN wasm_##name##_vec_t *out, const wasm_##name##_vec_t *in) {           \
    if (out) {                                                                 \
      *out = {0, nullptr};                                                     \
    }                                                                          \
    if (in && out && in->size < UINT32_MAX) {                                  \
      auto v = wasm::ownvec<Name>::make_uninitialized(in->size);               \
      for (size_t i = 0; i < v.size(); ++i) {                                  \
        v[i] = wasm::make_own(wasm_##name##_copy(in->data[i]));                \
      }                                                                        \
      *out = {v.size(), reinterpret_cast<wasm_##name##_t **>(v.release())};    \
    }                                                                          \
  }

// Helper macro for the WASM_DECLARE_TYPE implementation.
#define WASM_DECLARE_TYPE_IMPL(name, Name)                                     \
  WASM_DECLARE_OWN_IMPL(name, Name)                                            \
  WASM_DECLARE_VEC_PTR_IMPL(name, Name)                                        \
  WASMEDGE_CAPI_EXPORT wasm_##name##_t OWN *wasm_##name##_copy(                \
      const wasm_##name##_t *inst) {                                           \
    if (inst) {                                                                \
      return static_cast<wasm_##name##_t *>((inst->copy()).release());         \
    }                                                                          \
    return nullptr;                                                            \
  }

// Helper macro for the base of WASM_DECLARE_REF implementation.
#define WASM_DECLARE_REF_BASE_IMPL(name, Name)                                 \
  WASM_DECLARE_OWN_IMPL(name, Name)                                            \
  WASMEDGE_CAPI_EXPORT OWN wasm_##name##_t *wasm_##name##_copy(                \
      const wasm_##name##_t *t) {                                              \
    return static_cast<wasm_##name##_t *>(t->copy().release());                \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT bool wasm_##name##_same(const wasm_##name##_t *t1,      \
                                               const wasm_##name##_t *t2) {    \
    return t1->same(t2);                                                       \
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

// Helper macro for the WASM_DECLARE_REF implementation.
#define WASM_DECLARE_REF_IMPL(name, Name)                                      \
  WASM_DECLARE_REF_BASE_IMPL(name, Name)                                       \
  WASMEDGE_CAPI_EXPORT wasm_ref_t *wasm_##name##_as_ref(wasm_##name##_t *r) {  \
    return static_cast<wasm_ref_t *>(                                          \
        static_cast<wasm::Ref *>(static_cast<Name *>(r)));                     \
  }                                                                            \
  WASMEDGE_CAPI_EXPORT wasm_##name##_t *wasm_ref_as_##name(wasm_ref_t *r) {    \
    return static_cast<wasm_##name##_t *>(                                     \
        static_cast<Name *>(static_cast<wasm::Ref *>(r)));                     \
  }                                                                            \
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

// Helper macro for the WASM_DECLARE_SHARABLE_REF implementation.
#define WASM_DECLARE_SHARABLE_REF_IMPL(name, Name)                             \
  WASM_DECLARE_REF_IMPL(name, Name)                                            \
  WASM_DECLARE_OWN_IMPL(shared_##name, wasm::Shared<wasm::Module>)

// Helper macro for the wasm_externtype_t and wasm_extern_t conversions.
#define WASM_EXTERNTYPE_C_CONV(in, out, type, Type, connect, const_quant)      \
  WASMEDGE_CAPI_EXPORT const_quant wasm_##out##type##_t                        \
      *wasm_##in##type##_as_##out##type##connect##const_quant(                 \
          const_quant wasm_##in##type##_t *ptr) {                              \
    return static_cast<const_quant wasm_##out##type##_t *>(                    \
        static_cast<const_quant wasm::Extern##Type *>(ptr));                   \
  }

// Helper macro for the wasm::ExternType and wasm::Extern conversions.
#define WASM_EXTERNTYPE_CPP_CONV(name, Name, Type, enum, const_quant)          \
  const_quant wasm::Name##Type *wasm::Extern##Type::name() const_quant {       \
    if (kind() == wasm::ExternKind::enum) {                                    \
      return static_cast<const_quant wasm::Name##Type *>(this);                \
    }                                                                          \
    return nullptr;                                                            \
  }

inline wasm::Val conv_val_to_cpp(const wasm_val_t &v) {
  switch (v.kind) {
  case wasm_valkind_enum::WASM_I32:
    return wasm::Val(v.of.i32);
  case wasm_valkind_enum::WASM_I64:
    return wasm::Val(v.of.i64);
  case wasm_valkind_enum::WASM_F32:
    return wasm::Val(v.of.f32);
  case wasm_valkind_enum::WASM_F64:
    return wasm::Val(v.of.f64);
  case wasm_valkind_enum::WASM_ANYREF:
  case wasm_valkind_enum::WASM_FUNCREF: {
    if (v.of.ref) {
      return wasm::Val(v.of.ref->copy_ref());
    }
    return wasm::Val();
  }
  default:
    assumingUnreachable();
  }
}

inline wasm_val_t conv_val_to_c(const wasm::Val &v, wasm::ValKind k) {
  wasm_val_t r;
  switch (k) {
  case wasm::ValKind::I32:
    r.kind = wasm_valkind_enum::WASM_I32;
    r.of.i32 = v.i32();
    break;
  case wasm::ValKind::I64:
    r.kind = wasm_valkind_enum::WASM_I64;
    r.of.i64 = v.i64();
    break;
  case wasm::ValKind::F32:
    r.kind = wasm_valkind_enum::WASM_F32;
    r.of.f32 = v.f32();
    break;
  case wasm::ValKind::F64:
    r.kind = wasm_valkind_enum::WASM_F64;
    r.of.f64 = v.f64();
    break;
  case wasm::ValKind::FUNCREF:
    r.kind = wasm_valkind_enum::WASM_FUNCREF;
    r.of.ref = static_cast<wasm_ref_t *>(v.ref());
    break;
  case wasm::ValKind::ANYREF:
    r.kind = wasm_valkind_enum::WASM_ANYREF;
    r.of.ref = static_cast<wasm_ref_t *>(v.ref());
    break;
  default:
    assumingUnreachable();
  }
  return r;
}

// WasmEdge::ValVariant to wasm_val_t conversion.
wasm_val_t conv_cval_from_wasmedge(const WasmEdge::ValVariant &v,
                                   WasmEdge::ValType vt) {
  // Note: the conversion will not add the ref count of wasm_ref_t.
  wasm_val_t r;
  switch (vt) {
  case WasmEdge::ValType::I32:
    r.kind = wasm_valkind_enum::WASM_I32;
    r.of.i32 = v.get<uint32_t>();
    break;
  case WasmEdge::ValType::I64:
    r.kind = wasm_valkind_enum::WASM_I64;
    r.of.i64 = v.get<uint64_t>();
    break;
  case WasmEdge::ValType::F32:
    r.kind = wasm_valkind_enum::WASM_F32;
    r.of.f32 = v.get<float>();
    break;
  case WasmEdge::ValType::F64:
    r.kind = wasm_valkind_enum::WASM_F64;
    r.of.f64 = v.get<double>();
    break;
  case WasmEdge::ValType::FuncRef:
    r.kind = wasm_valkind_enum::WASM_FUNCREF;
    // TODO: implement this
    break;
  case WasmEdge::ValType::ExternRef:
    r.kind = wasm_valkind_enum::WASM_ANYREF;
    r.of.ref = WasmEdge::retrieveExternRef<wasm_ref_t *>(v);
    break;
  case WasmEdge::ValType::V128:
    // v128 is not supported in wasm C API yet.
  default:
    assumingUnreachable();
  }
  return r;
}

// WasmEdge::ValVariant to wasm::Val conversion.
wasm::Val conv_cppval_from_wasmedge(const WasmEdge::ValVariant &v,
                                    WasmEdge::ValType vt) {
  // Note: the conversion will add the ref count of wasm::Ref to make the
  // wasm::Val.
  switch (vt) {
  case WasmEdge::ValType::I32:
    return wasm::Val::i32(v.get<uint32_t>());
  case WasmEdge::ValType::I64:
    return wasm::Val::i64(v.get<uint64_t>());
  case WasmEdge::ValType::F32:
    return wasm::Val::f32(v.get<float>());
  case WasmEdge::ValType::F64:
    return wasm::Val::f64(v.get<double>());
  case WasmEdge::ValType::FuncRef:
    // TODO: implement this
    return wasm::Val();
  case WasmEdge::ValType::ExternRef:
    return wasm::Val::ref(
        WasmEdge::retrieveExternRef<wasm_ref_t *>(v)->copy_ref());
  case WasmEdge::ValType::V128:
    // v128 is not supported in wasm C API yet.
  default:
    assumingUnreachable();
  }
}

// wasm_val_t to WasmEdge::ValVariant conversion.
WasmEdge::ValVariant conv_cval_to_wasmedge(const wasm_val_t &v) {
  // Note: the conversion will not transfer the ownership of wasm_ref_t*.
  switch (v.kind) {
  case wasm_valkind_enum::WASM_I32:
    return v.of.i32;
  case wasm_valkind_enum::WASM_I64:
    return v.of.i64;
  case wasm_valkind_enum::WASM_F32:
    return v.of.f32;
  case wasm_valkind_enum::WASM_F64:
    return v.of.f64;
  case wasm_valkind_enum::WASM_ANYREF:
    return WasmEdge::ExternRef(v.of.ref);
  case wasm_valkind_enum::WASM_FUNCREF:
    if (v.of.ref && v.of.ref->category == wasm_category_enum::FUNC) {
      return WasmEdge::FuncRef(
          static_cast<wasm_func_t *>(static_cast<wasm::Ref *>(v.of.ref))->inst);
    } else {
      return WasmEdge::FuncRef();
    }
  default:
    assumingUnreachable();
  }
}

// wasm::Val to WasmEdge::ValVariant conversion.
WasmEdge::ValVariant conv_cppval_to_wasmedge(const wasm::Val &v,
                                             wasm::ValKind k) {
  // Note: the conversion will not transfer the ownership of wasm::Ref.
  switch (k) {
  case wasm::ValKind::I32:
    return v.i32();
  case wasm::ValKind::I64:
    return v.i64();
  case wasm::ValKind::F32:
    return v.f32();
  case wasm::ValKind::F64:
    return v.f64();
  case wasm::ValKind::ANYREF:
    return WasmEdge::ExternRef(v.ref());
  case wasm::ValKind::FUNCREF: {
    auto r = static_cast<wasm_ref_t *>(v.ref());
    if (r && r->category == wasm_category_enum::FUNC) {
      return WasmEdge::FuncRef(static_cast<wasm_func_t *>(v.ref())->inst);
    } else {
      return WasmEdge::FuncRef();
    }
  }
  default:
    assumingUnreachable();
  }
}

// allocmgr destructor backward implementation.
wasm_allocmgr_t::~wasm_allocmgr_t() {
  for (auto &&it : refcnts) {
    auto *ref = static_cast<wasm_ref_t *>(it.first);
    if (ref->finalizer) {
      ref->finalizer(ref->info);
    }
    it.second.second(ref);
  }
  refcnts.clear();
}

} // namespace

// The followings are the C API implementation.
#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> wasm_byte_vec_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_VEC_SCALAR_IMPL(byte, byte_t)

// <<<<<<<< wasm_byte_vec_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_config_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(config, wasm::Config)

WASMEDGE_CAPI_EXPORT OWN wasm_config_t *wasm_config_new() {
  return new (std::nothrow) wasm_config_t;
}

// <<<<<<<< wasm_config_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_engine_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(engine, wasm::Engine)

WASMEDGE_CAPI_EXPORT OWN wasm_engine_t *wasm_engine_new() {
  return new (std::nothrow) wasm_engine_t();
}

WASMEDGE_CAPI_EXPORT OWN wasm_engine_t *
wasm_engine_new_with_config(OWN wasm_config_t *conf) {
  if (conf) {
    return new (std::nothrow) wasm_engine_t(wasm::make_own<wasm::Config>(conf));
  } else {
    return wasm_engine_new();
  }
}

// <<<<<<<< wasm_engine_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_store_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(store, wasm::Store)

WASMEDGE_CAPI_EXPORT OWN wasm_store_t *wasm_store_new(wasm_engine_t *engine) {
  return static_cast<wasm_store_t *>(wasm::Store::make(engine).release());
}

// <<<<<<<< wasm_store_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_valtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(valtype, wasm::ValType)

WASMEDGE_CAPI_EXPORT OWN wasm_valtype_t *wasm_valtype_new(wasm_valkind_t kind) {
  // Return the pointer to the instances.
  // When delete the `wasm_valtype_t`, the `destroy()` function will do nothing
  // when the unique pointer releases the pointer.
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
  default:
    assumingUnreachable();
  };
}

WASM_DECLARE_C_GET_ATTR(valtype, wasm_valkind_t, kind, static, WASM_I32, , )

// <<<<<<<< wasm_valtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_functype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_functype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_globaltype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_globaltype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_tabletype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_tabletype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_memorytype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_memorytype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_externtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_TYPE_IMPL(externtype, wasm::ExternType)

WASM_DECLARE_C_GET_ATTR(externtype, wasm_externkind_t, kind, static,
                        WASM_EXTERN_FUNC, , )

WASM_EXTERNTYPE_C_CONV(func, extern, type, Type, , )
WASM_EXTERNTYPE_C_CONV(func, extern, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(global, extern, type, Type, , )
WASM_EXTERNTYPE_C_CONV(global, extern, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(table, extern, type, Type, , )
WASM_EXTERNTYPE_C_CONV(table, extern, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(memory, extern, type, Type, , )
WASM_EXTERNTYPE_C_CONV(memory, extern, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(extern, func, type, Type, , )
WASM_EXTERNTYPE_C_CONV(extern, func, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(extern, global, type, Type, , )
WASM_EXTERNTYPE_C_CONV(extern, global, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(extern, table, type, Type, , )
WASM_EXTERNTYPE_C_CONV(extern, table, type, Type, _, const)
WASM_EXTERNTYPE_C_CONV(extern, memory, type, Type, , )
WASM_EXTERNTYPE_C_CONV(extern, memory, type, Type, _, const)

// <<<<<<<< wasm_externtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_importtype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_importtype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_exporttype_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm_exporttype_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

////////////////////////////////////////////////////////////////////////////////

// Runtime Objects

// >>>>>>>> wasm_val_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_VEC_BASE_IMPL(val, wasm::Val, wasm::vec, wasm::vec, )

WASMEDGE_CAPI_EXPORT void wasm_val_vec_new(OWN wasm_val_vec_t *out, size_t size,
                                           OWN wasm_val_t const data[]) {
  if (out) {
    *out = {0, nullptr};
    if (size < UINT32_MAX) {
      auto v = wasm::vec<wasm::Val>::make_uninitialized(size);
      for (size_t i = 0; i < v.size(); ++i) {
        v[i] = conv_val_to_cpp(data[i]);
      }
      *out = {v.size(), reinterpret_cast<wasm_val_t *>(v.release())};
    }
  }
}

WASMEDGE_CAPI_EXPORT void wasm_val_vec_copy(OWN wasm_val_vec_t *out,
                                            const wasm_val_vec_t *in) {
  if (out) {
    *out = {0, nullptr};
  }
  if (in && out && in->size < UINT32_MAX) {
    auto v = wasm::vec<wasm::Val>::make_uninitialized(in->size);
    for (size_t i = 0; i < v.size(); ++i) {
      wasm_val_t val;
      wasm_val_copy(&in->data[i], &val);
      v[i] = conv_val_to_cpp(val);
    }
    *out = {v.size(), reinterpret_cast<wasm_val_t *>(v.release())};
  }
}

WASMEDGE_CAPI_EXPORT void wasm_val_delete(OWN wasm_val_t *v) {
  if (v && is_ref(static_cast<wasm::ValKind>(v->kind))) {
    wasm::make_own(v->of.ref);
  }
}

WASMEDGE_CAPI_EXPORT void wasm_val_copy(OWN wasm_val_t *out,
                                        const wasm_val_t *in) {
  if (out) {
    if (in) {
      *out = *in;
      if (wasm::is_ref(static_cast<wasm::ValKind>(in->kind)) && in->of.ref) {
        out->of.ref = static_cast<wasm_ref_t *>(in->of.ref->copy().release());
      }
    } else {
      out->kind = WASM_I32;
      out->of.i64 = 0;
    }
  }
}

// <<<<<<<< wasm_val_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_ref_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_BASE_IMPL(ref, wasm::Ref)

// <<<<<<<< wasm_ref_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_frame_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_OWN_IMPL(frame, wasm::Frame)
WASM_DECLARE_VEC_PTR_IMPL(frame, wasm::Frame)

WASMEDGE_CAPI_EXPORT OWN wasm_frame_t *
wasm_frame_copy(const wasm_frame_t *frame) {
  return static_cast<wasm_frame_t *>(frame->copy().release());
}

WASM_DECLARE_C_GET_ATTR(frame, wasm_instance_t *, instance, static, nullptr, , )
WASM_DECLARE_C_GET_ATTR(frame, uint32_t, func_index, static, 0, , )
WASM_DECLARE_C_GET_ATTR(frame, size_t, func_offset, static, 0, , )
WASM_DECLARE_C_GET_ATTR(frame, size_t, module_offset, static, 0, , )

// <<<<<<<< wasm_frame_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_trap_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(trap, wasm::Trap)

WASMEDGE_CAPI_EXPORT OWN wasm_trap_t *
wasm_trap_new(wasm_store_t *store, const wasm_message_t *message) {
  if (message) {
    wasm_byte_vec_t v;
    wasm_byte_vec_copy(&v, message);
    return new (std::nothrow)
        wasm_trap_t(store, wasm::vec<byte_t>::adopt(v.size, v.data));
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void wasm_trap_message(const wasm_trap_t *trap,
                                            OWN wasm_message_t *out) {
  if (out) {
    *out = {0, nullptr};
    if (trap) {
      auto v = trap->message();
      *out = {v.size(), reinterpret_cast<wasm_byte_t *>(v.release())};
    }
  }
}

WASMEDGE_CAPI_EXPORT OWN wasm_frame_t *
wasm_trap_origin(const wasm_trap_t *trap) {
  if (trap) {
    return static_cast<wasm_frame_t *>(trap->origin().release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void wasm_trap_trace(const wasm_trap_t *trap,
                                          OWN wasm_frame_vec_t *out) {
  if (out) {
    *out = {0, nullptr};
    if (trap) {
      auto v = trap->trace();
      *out = {v.size(), reinterpret_cast<wasm_frame_t **>(v.release())};
    }
  }
}

// <<<<<<<< wasm_trap_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_foreign_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(foreign, wasm::Foreign)

WASMEDGE_CAPI_EXPORT OWN wasm_foreign_t *wasm_foreign_new(wasm_store_t *store) {
  return static_cast<wasm_foreign_t *>(wasm::Foreign::make(store).release());
}

// <<<<<<<< wasm_foreign_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_module_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_SHARABLE_REF_IMPL(module, wasm::Module)

WASMEDGE_CAPI_EXPORT OWN wasm_module_t *
wasm_module_new(wasm_store_t *store, const wasm_byte_vec_t *binary) {
  if (store && binary) {
    wasm_byte_vec_t v;
    wasm_byte_vec_copy(&v, binary);
    auto mod = store->load.parseModule(WasmEdge::Span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(v.data), v.size));
    if (mod) {
      return new (std::nothrow) wasm_module_t(
          store, std::move(*mod), wasm::vec<byte_t>::adopt(v.size, v.data));
    }
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT bool wasm_module_validate(wasm_store_t *store,
                                               const wasm_byte_vec_t *binary) {
  if (store && binary) {
    auto mod = store->load.parseModule(WasmEdge::Span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(binary->data), binary->size));
    if (!mod) {
      return false;
    }
    auto valid = store->valid.validate(*mod->get());
    if (valid) {
      return true;
    }
  }
  return false;
}

WASMEDGE_CAPI_EXPORT void wasm_module_imports(const wasm_module_t *module,
                                              OWN wasm_importtype_vec_t *out) {
  if (out) {
    *out = {0, nullptr};
    if (module) {
      auto v = module->imports();
      *out = {v.size(), reinterpret_cast<wasm_importtype_t **>(v.release())};
    }
  }
}

WASMEDGE_CAPI_EXPORT void wasm_module_exports(const wasm_module_t *module,
                                              OWN wasm_exporttype_vec_t *out) {
  if (out) {
    *out = {0, nullptr};
    if (module) {
      auto v = module->exports();
      *out = {v.size(), reinterpret_cast<wasm_exporttype_t **>(v.release())};
    }
  }
}

WASMEDGE_CAPI_EXPORT void wasm_module_serialize(const wasm_module_t *module,
                                                OWN wasm_byte_vec_t *out) {
  if (out) {
    *out = {0, nullptr};
    if (module) {
      auto v = module->serialize();
      *out = {v.size(), reinterpret_cast<wasm_byte_t *>(v.release())};
    }
  }
}

WASMEDGE_CAPI_EXPORT OWN wasm_module_t *
wasm_module_deserialize(wasm_store_t *store, const wasm_byte_vec_t *binary) {
  return wasm_module_new(store, binary);
}

WASMEDGE_CAPI_EXPORT OWN wasm_shared_module_t *
wasm_module_share(const wasm_module_t *module) {
  if (module) {
    return static_cast<wasm_shared_module_t *>(module->share().release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT OWN wasm_module_t *
wasm_module_obtain(wasm_store_t *store, const wasm_shared_module_t *shared) {
  if (store && shared) {
    return static_cast<wasm_module_t *>(
        wasm::Module::obtain(store, shared).release());
  }
  return nullptr;
}

// <<<<<<<< wasm_module_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_func_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(func, wasm::Func)

WASMEDGE_CAPI_EXPORT OWN wasm_func_t *
wasm_func_new(wasm_store_t *store, const wasm_functype_t *type,
              wasm_func_callback_t callback) {
  if (store && type && callback) {
    return new (std::nothrow) wasm_func_t(store, *type, callback);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT OWN wasm_func_t *
wasm_func_new_with_env(wasm_store_t *store, const wasm_functype_t *type,
                       wasm_func_callback_with_env_t callback, void *env,
                       void (*finalizer)(void *)) {
  if (store && type && callback) {
    return new (std::nothrow)
        wasm_func_t(store, *type, callback, env, finalizer);
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT OWN wasm_functype_t *
wasm_func_type(const wasm_func_t *func) {
  if (func) {
    return static_cast<wasm_functype_t *>(func->type().release());
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(func, size_t, param_arity, static, 0, , )
WASM_DECLARE_C_GET_ATTR(func, size_t, result_arity, static, 0, , )

WASMEDGE_CAPI_EXPORT OWN wasm_trap_t *
wasm_func_call(const wasm_func_t *, const wasm_val_vec_t *, wasm_val_vec_t *) {
  // TODO: implement this
  return nullptr;
}

// <<<<<<<< wasm_func_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_global_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(global, wasm::Global)

WASMEDGE_CAPI_EXPORT OWN wasm_global_t *
wasm_global_new(wasm_store_t *store, const wasm_globaltype_t *type,
                const wasm_val_t *val) {
  if (store && type && val) {
    return static_cast<wasm_global_t *>(
        wasm::Global::make(store, type, conv_val_to_cpp(*val)).release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT OWN wasm_globaltype_t *
wasm_global_type(const wasm_global_t *global) {
  if (global) {
    return static_cast<wasm_globaltype_t *>(global->type().release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT void wasm_global_get(const wasm_global_t *global,
                                          OWN wasm_val_t *out) {
  if (global && out) {
    *out = conv_val_to_c(
        global->get(),
        conv_valtype_from_wasmedge(global->inst.getGlobalType().getValType()));
  }
}

WASMEDGE_CAPI_EXPORT void wasm_global_set(wasm_global_t *global,
                                          const wasm_val_t *val) {
  if (global && val) {
    global->set(conv_val_to_cpp(*val));
  }
}

// <<<<<<<< wasm_global_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_table_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(table, wasm::Table)

WASMEDGE_CAPI_EXPORT OWN wasm_table_t *
wasm_table_new(wasm_store_t *store, const wasm_tabletype_t *type,
               wasm_ref_t *ref) {
  return static_cast<wasm_table_t *>(
      wasm::Table::make(store, type, ref).release());
}

WASMEDGE_CAPI_EXPORT OWN wasm_tabletype_t *
wasm_table_type(const wasm_table_t *table) {
  if (table) {
    return static_cast<wasm_tabletype_t *>(table->type().release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT OWN wasm_ref_t *wasm_table_get(const wasm_table_t *table,
                                                    wasm_table_size_t index) {
  if (table) {
    return static_cast<wasm_ref_t *>(table->get(index).release());
  }
  return nullptr;
}

WASMEDGE_CAPI_EXPORT bool
wasm_table_set(wasm_table_t *table, wasm_table_size_t index, wasm_ref_t *ref) {
  if (table) {
    return table->set(index, ref);
  }
  return false;
}

WASM_DECLARE_C_GET_ATTR(table, wasm_table_size_t, size, static, 0, , )

WASMEDGE_CAPI_EXPORT bool
wasm_table_grow(wasm_table_t *table, wasm_table_size_t delta, wasm_ref_t *ref) {
  if (table) {
    return table->grow(delta, ref);
  }
  return false;
}

// <<<<<<<< wasm_table_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_memory_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(memory, wasm::Memory)

WASMEDGE_CAPI_EXPORT OWN wasm_memory_t *
wasm_memory_new(wasm_store_t *store, const wasm_memorytype_t *type) {
  return static_cast<wasm_memory_t *>(
      wasm::Memory::make(store, type).release());
}

WASMEDGE_CAPI_EXPORT OWN wasm_memorytype_t *
wasm_memory_type(const wasm_memory_t *memory) {
  if (memory) {
    return static_cast<wasm_memorytype_t *>(memory->type().release());
  }
  return nullptr;
}

WASM_API_EXTERN byte_t *wasm_memory_data(wasm_memory_t *memory) {
  if (memory) {
    return static_cast<byte_t *>(memory->data());
  }
  return nullptr;
}

WASM_DECLARE_C_GET_ATTR(memory, size_t, data_size, static, 0, , )
WASM_DECLARE_C_GET_ATTR(memory, wasm_memory_pages_t, size, static, 0, , )

WASMEDGE_CAPI_EXPORT bool wasm_memory_grow(wasm_memory_t *memory,
                                           wasm_memory_pages_t delta) {
  if (memory) {
    return memory->grow(static_cast<wasm::Memory::pages_t>(delta));
  }
  return false;
}

// <<<<<<<< wasm_memory_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_extern_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(extern, wasm::Extern)
WASM_DECLARE_VEC_PTR_IMPL(extern, wasm::Extern)

WASM_DECLARE_C_GET_ATTR(extern, wasm_externkind_t, kind, static,
                        WASM_EXTERN_FUNC, , )

WASMEDGE_CAPI_EXPORT OWN wasm_externtype_t *
wasm_extern_type(const wasm_extern_t *external) {
  return reinterpret_cast<wasm_externtype_t *>(external->type().release());
}

WASM_EXTERNTYPE_C_CONV(func, extern, , , , )
WASM_EXTERNTYPE_C_CONV(func, extern, , , _, const)
WASM_EXTERNTYPE_C_CONV(global, extern, , , , )
WASM_EXTERNTYPE_C_CONV(global, extern, , , _, const)
WASM_EXTERNTYPE_C_CONV(table, extern, , , , )
WASM_EXTERNTYPE_C_CONV(table, extern, , , _, const)
WASM_EXTERNTYPE_C_CONV(memory, extern, , , , )
WASM_EXTERNTYPE_C_CONV(memory, extern, , , _, const)
WASM_EXTERNTYPE_C_CONV(extern, func, , , , )
WASM_EXTERNTYPE_C_CONV(extern, func, , , _, const)
WASM_EXTERNTYPE_C_CONV(extern, global, , , , )
WASM_EXTERNTYPE_C_CONV(extern, global, , , _, const)
WASM_EXTERNTYPE_C_CONV(extern, table, , , , )
WASM_EXTERNTYPE_C_CONV(extern, table, , , _, const)
WASM_EXTERNTYPE_C_CONV(extern, memory, , , , )
WASM_EXTERNTYPE_C_CONV(extern, memory, , , _, const)

// <<<<<<<< wasm_extern_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm_instance_t functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

WASM_DECLARE_REF_IMPL(instance, wasm::Instance)

WASMEDGE_CAPI_EXPORT OWN wasm_instance_t *
wasm_instance_new(wasm_store_t *store, const wasm_module_t *module,
                  const wasm_extern_vec_t *imports, OWN wasm_trap_t **trap) {
  // TODO: implement this
  wasm::own<wasm::Trap> error;
  auto imports_ = reinterpret_cast<const wasm::vec<wasm::Extern *> *>(imports);
  auto instance = static_cast<wasm_instance_t *>(
      wasm::Instance::make(store, module, *imports_, &error).release());
  if (trap)
    *trap = static_cast<wasm_trap_t *>(error.release());
  return instance;
}

WASMEDGE_CAPI_EXPORT void wasm_instance_exports(const wasm_instance_t *instance,
                                                OWN wasm_extern_vec_t *out) {
  if (out) {
    auto exports = instance->exports();
    *out = {exports.size(),
            reinterpret_cast<wasm_extern_t **>(exports.release())};
  }
}

// <<<<<<<< wasm_instance_t functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef OWN
#undef OWN
#endif

#ifdef __cplusplus
} // extern "C"
#endif

// The followings are the C++ API implementation.

// >>>>>>>> wasm::Config functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Config> wasm::Config::make() {
  return wasm::make_own(new (std::nothrow) wasm_config_t);
}

void wasm::Config::destroy() { delete static_cast<wasm_config_t *>(this); }

// <<<<<<<< wasm::Config functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Engine functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Engine> wasm::Engine::make(wasm::own<wasm::Config> &&config) {
  if (config.get()) {
    return wasm::make_own(new (std::nothrow) wasm_engine_t(std::move(config)));
  }
  return wasm::make_own(new (std::nothrow) wasm_engine_t());
}

void wasm::Engine::destroy() { delete static_cast<wasm_engine_t *>(this); }

// <<<<<<<< wasm::Engine functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

wasm::own<wasm::Store> wasm::Store::make(wasm::Engine *engine) {
  if (engine) {
    return wasm::make_own(new (std::nothrow) wasm_store_t(engine));
  }
  return nullptr;
}

void wasm::Store::destroy() { delete static_cast<wasm_store_t *>(this); }

// <<<<<<<< wasm::Store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::ValType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ValType::destroy() {
  // Not to delete the struct because the ValTypes are global variables.
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
  default:
    assumingUnreachable();
  };
}

wasm::own<wasm::ValType> wasm::ValType::copy() const { return make(kind()); }

wasm::ValKind wasm::ValType::kind() const {
  return (static_cast<const wasm_valtype_t *>(this))->mkind;
}

// <<<<<<<< wasm::ValType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::FuncType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::FuncType::destroy() { delete static_cast<wasm_functype_t *>(this); }

wasm::own<wasm::FuncType>
wasm::FuncType::make(wasm::ownvec<wasm::ValType> &&params,
                     wasm::ownvec<wasm::ValType> &&results) {
  // Check the inputs are valid.
  if (params && results) {
    return wasm::make_own(new (std::nothrow) wasm_functype_t(
        std::move(params), std::move(results)));
  }
  return nullptr;
}

wasm::own<wasm::FuncType> wasm::FuncType::copy() const {
  // The `params` and `results` vectors here must be valid.
  return make(params().deep_copy(), results().deep_copy());
}

const wasm::ownvec<wasm::ValType> &wasm::FuncType::params() const {
  return (static_cast<const wasm_functype_t *>(this))->mparams;
}

const wasm::ownvec<wasm::ValType> &wasm::FuncType::results() const {
  return (static_cast<const wasm_functype_t *>(this))->mresults;
}

// <<<<<<<< wasm::FuncType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::GlobalType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::GlobalType::destroy() {
  delete static_cast<wasm_globaltype_t *>(this);
}

wasm::own<wasm::GlobalType>
wasm::GlobalType::make(wasm::own<wasm::ValType> &&content,
                       wasm::Mutability mutability) {
  return wasm::make_own(new (std::nothrow)
                            wasm_globaltype_t(std::move(content), mutability));
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

// <<<<<<<< wasm::GlobalType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::TableType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::TableType::destroy() {
  delete static_cast<wasm_tabletype_t *>(this);
}

wasm::own<wasm::TableType>
wasm::TableType::make(wasm::own<wasm::ValType> &&element, wasm::Limits limits) {
  return wasm::make_own(new (std::nothrow)
                            wasm_tabletype_t(std::move(element), limits));
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

// <<<<<<<< wasm::TableType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::MemoryType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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

// <<<<<<<< wasm::MemoryType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::ExternType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
  default:
    assumingUnreachable();
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
  default:
    assumingUnreachable();
  }
}

wasm::ExternKind wasm::ExternType::kind() const {
  return static_cast<const wasm_externtype_t *>(this)->mkind;
}

WASM_EXTERNTYPE_CPP_CONV(func, Func, Type, FUNC, )
WASM_EXTERNTYPE_CPP_CONV(func, Func, Type, FUNC, const)
WASM_EXTERNTYPE_CPP_CONV(global, Global, Type, GLOBAL, )
WASM_EXTERNTYPE_CPP_CONV(global, Global, Type, GLOBAL, const)
WASM_EXTERNTYPE_CPP_CONV(table, Table, Type, TABLE, )
WASM_EXTERNTYPE_CPP_CONV(table, Table, Type, TABLE, const)
WASM_EXTERNTYPE_CPP_CONV(memory, Memory, Type, MEMORY, )
WASM_EXTERNTYPE_CPP_CONV(memory, Memory, Type, MEMORY, const)

// <<<<<<<< wasm::ExternType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::ImportType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ImportType::destroy() {
  delete static_cast<wasm_importtype_t *>(this);
}

wasm::own<wasm::ImportType>
wasm::ImportType::make(wasm::Name &&module, wasm::Name &&name,
                       wasm::own<wasm::ExternType> &&type) {
  return wasm::make_own(new (std::nothrow) wasm_importtype_t(
      std::move(module), std::move(name), std::move(type)));
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

// <<<<<<<< wasm::ImportType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::ExportType functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::ExportType::destroy() {
  delete static_cast<wasm_exporttype_t *>(this);
}

wasm::own<wasm::ExportType>
wasm::ExportType::make(wasm::Name &&name, wasm::own<wasm::ExternType> &&type) {
  return wasm::make_own(
      new (std::nothrow) wasm_exporttype_t(std::move(name), std::move(type)));
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

// <<<<<<<< wasm::ExportType functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

////////////////////////////////////////////////////////////////////////////////

// Runtime Objects

// >>>>>>>> wasm::Val functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// The `wasm::Val` class has been fully implemented in header.

// <<<<<<<< wasm::Val functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Ref functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Ref::destroy() {
  static_cast<wasm_ref_t *>(this)->~wasm_ref_base_t<wasm_ref_t>();
}

wasm::own<wasm::Ref> wasm::Ref::copy() const {
  return static_cast<const wasm_ref_t *>(this)->copy_ref();
}

bool wasm::Ref::same(const wasm::Ref *that) const {
  return static_cast<const wasm_ref_t *>(this)->inner ==
         static_cast<const wasm_ref_t *>(that)->inner;
}

void *wasm::Ref::get_host_info() const {
  return static_cast<const wasm_ref_t *>(this)->info;
}

void wasm::Ref::set_host_info(void *info, void (*finalizer)(void *)) {
  static_cast<wasm_ref_t *>(this)->info = info;
  static_cast<wasm_ref_t *>(this)->finalizer = finalizer;
}

// <<<<<<<< wasm::Ref functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Frame functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Frame::destroy() { delete static_cast<wasm_frame_t *>(this); }

wasm::own<wasm::Frame> wasm::Frame::copy() const {
  return wasm::make_own(new (std::nothrow) wasm_frame_t(
      *static_cast<const wasm_frame_t *>(this)));
}

wasm::Instance *wasm::Frame::instance() const {
  // TODO: Not supported yet.
  return nullptr;
}

uint32_t wasm::Frame::func_index() const {
  return static_cast<const wasm_frame_t *>(this)->funcidx;
}

size_t wasm::Frame::func_offset() const {
  return static_cast<const wasm_frame_t *>(this)->funcoff;
}

size_t wasm::Frame::module_offset() const {
  return static_cast<const wasm_frame_t *>(this)->moduleoff;
}

// <<<<<<<< wasm::Frame functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Trap functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Trap::destroy() {
  static_cast<wasm_trap_t *>(this)->~wasm_ref_base_t<wasm_trap_t>();
}

wasm::own<wasm::Trap> wasm::Trap::make(wasm::Store *store,
                                       const wasm::Message &msg) {
  if (store) {
    return wasm::make_own(new (std::nothrow) wasm_trap_t(store, msg.copy()));
  }
  return nullptr;
}

wasm::own<wasm::Trap> wasm::Trap::copy() const {
  return static_cast<const wasm_trap_t *>(this)->copy_ref();
}

wasm::Message wasm::Trap::message() const {
  return static_cast<const wasm_trap_t *>(this)->mmsg.copy();
}

wasm::own<wasm::Frame> wasm::Trap::origin() const {
  // TODO: Not supported yet.
  return nullptr;
}

wasm::ownvec<wasm::Frame> wasm::Trap::trace() const {
  // TODO: Not supported yet.
  return wasm::ownvec<wasm::Frame>::make();
}

// <<<<<<<< wasm::Trap functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Foreign functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Foreign::destroy() {
  static_cast<wasm_foreign_t *>(this)->~wasm_ref_base_t<wasm_foreign_t>();
}

wasm::own<wasm::Foreign> wasm::Foreign::make(wasm::Store *store) {
  if (store) {
    return wasm::make_own(new (std::nothrow) wasm_foreign_t(store));
  }
  return nullptr;
}

wasm::own<wasm::Foreign> wasm::Foreign::copy() const {
  return static_cast<const wasm_foreign_t *>(this)->copy_ref();
}

// <<<<<<<< wasm::Foreign functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Modules functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Module::destroy() {
  static_cast<wasm_module_t *>(this)->~wasm_ref_base_t<wasm_module_t>();
}

bool wasm::Module::validate(wasm::Store *store,
                            const wasm::vec<byte_t> &binary) {
  if (store) {
    auto mod = static_cast<wasm_store_t *>(store)->load.parseModule(
        WasmEdge::Span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(binary.get()), binary.size()));
    if (!mod) {
      return false;
    }
    auto valid =
        static_cast<wasm_store_t *>(store)->valid.validate(*mod->get());
    if (valid) {
      return true;
    }
  }
  return false;
}

wasm::own<wasm::Module> wasm::Module::make(wasm::Store *store,
                                           const wasm::vec<byte_t> &binary) {
  if (store) {
    auto mod = static_cast<wasm_store_t *>(store)->load.parseModule(
        WasmEdge::Span<const uint8_t>(
            reinterpret_cast<const uint8_t *>(binary.get()), binary.size()));
    if (mod) {
      return wasm::make_own(new (std::nothrow) wasm_module_t(
          store, std::move(*mod), binary.copy()));
    }
  }
  return nullptr;
}

wasm::own<wasm::Module> wasm::Module::copy() const {
  return static_cast<const wasm_module_t *>(this)->copy_ref();
}

wasm::ownvec<wasm::ImportType> wasm::Module::imports() const {
  const auto &importvec = static_cast<const wasm_module_t *>(this)
                              ->ast->getImportSection()
                              .getContent();
  // The size of std::vector is not necessary to be checked.
  auto v = wasm::ownvec<wasm::ImportType>::make_uninitialized(importvec.size());
  for (size_t i = 0; i < v.size(); ++i) {
    auto modname = wasm::Name::make(std::string(importvec[i].getModuleName()));
    auto extname =
        wasm::Name::make(std::string(importvec[i].getExternalName()));
    wasm::own<wasm::ExternType> type;
    switch (importvec[i].getExternalType()) {
    case WasmEdge::ExternalType::Function: {
      auto typeidx = importvec[i].getExternalFuncTypeIdx();
      const auto &functype = static_cast<const wasm_module_t *>(this)
                                 ->ast->getTypeSection()
                                 .getContent()[typeidx];
      type = make_functype_from_wasmedge(functype);
      break;
    }
    case WasmEdge::ExternalType::Table: {
      type = make_tabletype_from_wasmedge(importvec[i].getExternalTableType());
      break;
    }
    case WasmEdge::ExternalType::Memory: {
      type =
          make_memorytype_from_wasmedge(importvec[i].getExternalMemoryType());
      break;
    }
    case WasmEdge::ExternalType::Global: {
      type =
          make_globaltype_from_wasmedge(importvec[i].getExternalGlobalType());
      break;
    }
    default:
      assumingUnreachable();
    }
    v[i] = wasm::make_own(new (std::nothrow) wasm_importtype_t(
        std::move(modname), std::move(extname), std::move(type)));
  }
  return v;
}

wasm::ownvec<wasm::ExportType> wasm::Module::exports() const {
  // Accumulate the imported instances.
  const auto &importvec = static_cast<const wasm_module_t *>(this)
                              ->ast->getImportSection()
                              .getContent();
  size_t impfunc = 0;
  size_t imptable = 0;
  size_t impmemory = 0;
  size_t impglobal = 0;
  for (size_t i = 0; i < importvec.size(); ++i) {
    switch (importvec[i].getExternalType()) {
    case WasmEdge::ExternalType::Function:
      ++impfunc;
      break;
    case WasmEdge::ExternalType::Table:
      ++imptable;
      break;
    case WasmEdge::ExternalType::Memory:
      ++impmemory;
      break;
    case WasmEdge::ExternalType::Global:
      ++impglobal;
      break;
    default:
      assumingUnreachable();
    }
  }

  const auto &exportvec = static_cast<const wasm_module_t *>(this)
                              ->ast->getExportSection()
                              .getContent();
  // The size of std::vector is not necessary to be checked.
  auto v = wasm::ownvec<wasm::ExportType>::make_uninitialized(exportvec.size());
  for (size_t i = 0; i < v.size(); ++i) {
    auto extname =
        wasm::Name::make(std::string(exportvec[i].getExternalName()));
    auto extidx = exportvec[i].getExternalIndex();
    wasm::own<wasm::ExternType> type;
    switch (exportvec[i].getExternalType()) {
      // The real index = external_index - import_num.
    case WasmEdge::ExternalType::Function: {
      auto typeidx = static_cast<const wasm_module_t *>(this)
                         ->ast->getFunctionSection()
                         .getContent()[extidx - impfunc];
      const auto &functype = static_cast<const wasm_module_t *>(this)
                                 ->ast->getTypeSection()
                                 .getContent()[typeidx];
      type = make_functype_from_wasmedge(functype);
      break;
    }
    case WasmEdge::ExternalType::Table: {
      const auto &tabletype = static_cast<const wasm_module_t *>(this)
                                  ->ast->getTableSection()
                                  .getContent()[extidx - imptable];
      type = make_tabletype_from_wasmedge(tabletype);
      break;
    }
    case WasmEdge::ExternalType::Memory: {
      const auto &memorytype = static_cast<const wasm_module_t *>(this)
                                   ->ast->getMemorySection()
                                   .getContent()[extidx - impmemory];
      type = make_memorytype_from_wasmedge(memorytype);
      break;
    }
    case WasmEdge::ExternalType::Global: {
      const auto &globaltype = static_cast<const wasm_module_t *>(this)
                                   ->ast->getGlobalSection()
                                   .getContent()[extidx - impglobal]
                                   .getGlobalType();
      type = make_globaltype_from_wasmedge(globaltype);
      break;
    }
    default:
      assumingUnreachable();
    }
    v[i] = wasm::make_own(new (std::nothrow) wasm_exporttype_t(
        std::move(extname), std::move(type)));
  }
  return v;
}

wasm::own<wasm::Shared<wasm::Module>> wasm::Module::share() const {
  return wasm::make_own(new (std::nothrow) wasm_shared_module_t(
      static_cast<const wasm_module_t *>(this)->binary.copy()));
}

wasm::own<wasm::Module>
wasm::Module::obtain(wasm::Store *store,
                     const wasm::Shared<wasm::Module> *module) {
  return wasm::Module::deserialize(
      store, static_cast<const wasm_shared_module_t *>(module)->binary);
}

wasm::vec<byte_t> wasm::Module::serialize() const {
  // TODO: Refine this after the implementation of serialization in WasmEdge.
  return static_cast<const wasm_module_t *>(this)->binary.copy();
}

wasm::own<wasm::Module>
wasm::Module::deserialize(wasm::Store *store,
                          const wasm::vec<byte_t> &vec_byte) {
  // TODO: Refine this after the implementation of deserialization in WasmEdge.
  return make(store, vec_byte);
}

// <<<<<<<< wasm::Modules functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Shared functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Shared<wasm::Module>::destroy() {
  delete static_cast<wasm_shared_module_t *>(this);
}

// <<<<<<<< wasm::Shared functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Func functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Func::destroy() {
  static_cast<wasm_func_t *>(this)->~wasm_ref_base_t<wasm_func_t>();
}

wasm::own<wasm::Func> wasm::Func::make(wasm::Store *store,
                                       const wasm::FuncType *type,
                                       wasm::Func::callback callback) {
  if (store && type && callback) {
    return wasm::make_own(new (std::nothrow)
                              wasm_func_t(store, *type, callback));
  }
  return nullptr;
}

wasm::own<wasm::Func> wasm::Func::make(wasm::Store *store,
                                       const wasm::FuncType *type,
                                       wasm::Func::callback_with_env callback,
                                       void *env, void (*finalizer)(void *)) {
  if (store && type && callback) {
    return wasm::make_own(
        new (std::nothrow) wasm_func_t(store, *type, callback, env, finalizer));
  }
  return nullptr;
}

wasm::own<wasm::Func> wasm::Func::copy() const {
  return static_cast<const wasm_func_t *>(this)->copy_ref();
}

wasm::own<wasm::FuncType> wasm::Func::type() const {
  if (static_cast<const wasm_func_t *>(this)->inst) {
    const auto &type =
        static_cast<const wasm_func_t *>(this)->inst->getFuncType();
    auto vp =
        wasm::ownvec<ValType>::make_uninitialized(type.getParamTypes().size());
    auto vr =
        wasm::ownvec<ValType>::make_uninitialized(type.getReturnTypes().size());
    for (size_t i = 0; i < type.getParamTypes().size(); ++i) {
      vp[i] = wasm::ValType::make(
          conv_valtype_from_wasmedge(type.getParamTypes()[i]));
    }
    for (size_t i = 0; i < type.getReturnTypes().size(); ++i) {
      vr[i] = wasm::ValType::make(
          conv_valtype_from_wasmedge(type.getReturnTypes()[i]));
    }
    return wasm::FuncType::make(std::move(vp), std::move(vr));
  }
  return wasm::FuncType::make();
}

size_t wasm::Func::param_arity() const {
  const auto *inst = static_cast<const wasm_func_t *>(this)->inst;
  if (inst) {
    return static_cast<size_t>(inst->getFuncType().getParamTypes().size());
  }
  return 0;
}

size_t wasm::Func::result_arity() const {
  const auto *inst = static_cast<const wasm_func_t *>(this)->inst;
  if (inst) {
    return static_cast<size_t>(inst->getFuncType().getReturnTypes().size());
  }
  return 0;
}

wasm::own<wasm::Trap> wasm::Func::call(const wasm::vec<wasm::Val> &,
                                       wasm::vec<wasm::Val> &) const {
  // TODO: implement this
  return wasm::own<wasm::Trap>();
}

// <<<<<<<< wasm::Func functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Global functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Global::destroy() {
  static_cast<wasm_global_t *>(this)->~wasm_ref_base_t<wasm_global_t>();
}

wasm::own<wasm::Global> wasm::Global::make(wasm::Store *store,
                                           const wasm::GlobalType *type,
                                           const wasm::Val &val) {
  if (store && type) {
    return wasm::make_own(new (std::nothrow) wasm_global_t(store, *type, val));
  }
  return nullptr;
}

wasm::own<wasm::Global> wasm::Global::copy() const {
  return static_cast<const wasm_global_t *>(this)->copy_ref();
}

wasm::own<wasm::GlobalType> wasm::Global::type() const {
  const auto &type =
      static_cast<const wasm_global_t *>(this)->inst.getGlobalType();
  return wasm::GlobalType::make(
      wasm::ValType::make(conv_valtype_from_wasmedge(type.getValType())),
      conv_valmut_from_wasmedge(type.getValMut()));
}

wasm::Val wasm::Global::get() const {
  const auto &inst = static_cast<const wasm_global_t *>(this)->inst;
  return conv_cppval_from_wasmedge(inst.getValue(),
                                   inst.getGlobalType().getValType());
}

void wasm::Global::set(const wasm::Val &val) {
  // Replace the kept init value here. If the value type is a reference, this
  // will help to keep the reference count.
  auto *global = static_cast<wasm_global_t *>(this);
  global->keepval = val.copy();
  global->inst.getValue() = conv_cppval_to_wasmedge(
      val,
      conv_valtype_from_wasmedge(global->inst.getGlobalType().getValType()));
}

// <<<<<<<< wasm::Global functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Table functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Table::destroy() {
  static_cast<wasm_table_t *>(this)->~wasm_ref_base_t<wasm_table_t>();
}

wasm::own<wasm::Table> wasm::Table::make(wasm::Store *store,
                                         const wasm::TableType *type,
                                         const wasm::Ref *init) {
  if (store && type) {
    return wasm::make_own(new (std::nothrow) wasm_table_t(store, *type, init));
  }
  return nullptr;
}

wasm::own<wasm::Table> wasm::Table::copy() const {
  return static_cast<const wasm_table_t *>(this)->copy_ref();
}

wasm::own<wasm::TableType> wasm::Table::type() const {
  const auto &type =
      static_cast<const wasm_table_t *>(this)->inst.getTableType();
  return wasm::TableType::make(
      wasm::ValType::make(conv_reftype_from_wasmedge(type.getRefType())),
      type.getLimit().hasMax()
          ? wasm::Limits(type.getLimit().getMin(), type.getLimit().getMax())
          : wasm::Limits(type.getLimit().getMin()));
}

wasm::own<wasm::Ref> wasm::Table::get(wasm::Table::size_t index) const {
  const auto &inst = static_cast<const wasm_table_t *>(this)->inst;
  if (auto res = inst.getRefAddr(static_cast<uint32_t>(index));
      res && !WasmEdge::isNullRef(*res)) {
    if (inst.getTableType().getRefType() == WasmEdge::RefType::FuncRef) {
      // TODO: implement this
    } else {
      return WasmEdge::retrieveExternRef<wasm_ref_t *>(*res)->copy_ref();
    }
  }
  return nullptr;
}

bool wasm::Table::set(wasm::Table::size_t index, const wasm::Ref *ref) {
  auto &inst = static_cast<wasm_table_t *>(this)->inst;
  if (inst.getTableType().getRefType() == WasmEdge::RefType::ExternRef) {
    // For the externref of the element type, set the ref.
    if (inst.setRefAddr(static_cast<uint32_t>(index),
                        WasmEdge::ExternRef(const_cast<wasm::Ref *>(ref)))) {
      return true;
    }
  } else {
    // For the funcref of the element type, check the input ref is a funcref.
    if (ref) {
      if (static_cast<const wasm_ref_t *>(ref)->category ==
          wasm_category_enum::FUNC) {
        if (inst.setRefAddr(index,
                            WasmEdge::FuncRef(
                                static_cast<const wasm_func_t *>(ref)->inst))) {
          return true;
        }
      }
    } else {
      if (inst.setRefAddr(index, WasmEdge::FuncRef())) {
        return true;
      }
    }
  }
  return false;
}

wasm::Table::size_t wasm::Table::size() const {
  return static_cast<wasm::Table::size_t>(
      static_cast<const wasm_table_t *>(this)->inst.getSize());
}

bool wasm::Table::grow(wasm::Table::size_t delta, const wasm::Ref *ref) {
  size_t orig = size();
  if (auto res = static_cast<wasm_table_t *>(this)->inst.growTable(
          static_cast<uint32_t>(delta));
      res) {
    for (size_t i = orig; i < orig + delta; ++i) {
      set(i, ref);
    }
    return true;
  } else {
    spdlog::error(WasmEdge::ErrCode::Value::TableOutOfBounds);
    return false;
  }
}

// <<<<<<<< wasm::Table functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Memory functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Memory::destroy() {
  static_cast<wasm_memory_t *>(this)->~wasm_ref_base_t<wasm_memory_t>();
}

wasm::own<wasm::Memory> wasm::Memory::make(wasm::Store *store,
                                           const wasm::MemoryType *type) {
  if (store && type) {
    return wasm::make_own(new (std::nothrow) wasm_memory_t(store, *type));
  }
  return nullptr;
}

wasm::own<wasm::Memory> wasm::Memory::copy() const {
  return static_cast<const wasm_memory_t *>(this)->copy_ref();
}

wasm::own<wasm::MemoryType> wasm::Memory::type() const {
  const auto &type =
      static_cast<const wasm_memory_t *>(this)->inst.getMemoryType();
  return wasm::MemoryType::make(
      type.getLimit().hasMax()
          ? wasm::Limits(type.getLimit().getMin(), type.getLimit().getMax())
          : wasm::Limits(type.getLimit().getMin()));
}

byte_t *wasm::Memory::data() const {
  return static_cast<const wasm_memory_t *>(this)->inst.getPointer<byte_t *>(
      0, data_size());
}

size_t wasm::Memory::data_size() const {
  return static_cast<size_t>(
      static_cast<const wasm_memory_t *>(this)->inst.getPageSize() *
      wasm::Memory::page_size);
}

wasm::Memory::pages_t wasm::Memory::size() const {
  return static_cast<wasm::Memory::pages_t>(
      static_cast<const wasm_memory_t *>(this)->inst.getPageSize());
}

bool wasm::Memory::grow(wasm::Memory::pages_t delta) {
  if (auto res = static_cast<wasm_memory_t *>(this)->inst.growPage(
          static_cast<uint32_t>(delta));
      res) {
    return true;
  } else {
    spdlog::error(WasmEdge::ErrCode::Value::MemoryOutOfBounds);
    return false;
  }
}

// <<<<<<<< wasm::Memory functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Extern functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Extern::destroy() {
  static_cast<wasm_extern_t *>(this)->~wasm_ref_base_t<wasm_extern_t>();
}

wasm::own<wasm::Extern> wasm::Extern::copy() const {
  return static_cast<const wasm_extern_t *>(this)->copy_ref();
}

wasm::ExternKind wasm::Extern::kind() const {
  return static_cast<const wasm_extern_t *>(this)->mkind;
}

wasm::own<wasm::ExternType> wasm::Extern::type() const {
  switch (kind()) {
  case wasm::ExternKind::FUNC:
    return func()->type();
  case wasm::ExternKind::GLOBAL:
    return global()->type();
  case wasm::ExternKind::TABLE:
    return table()->type();
  case wasm::ExternKind::MEMORY:
    return memory()->type();
  default:
    assumingUnreachable();
  }
}

WASM_EXTERNTYPE_CPP_CONV(func, Func, , FUNC, )
WASM_EXTERNTYPE_CPP_CONV(func, Func, , FUNC, const)
WASM_EXTERNTYPE_CPP_CONV(global, Global, , GLOBAL, )
WASM_EXTERNTYPE_CPP_CONV(global, Global, , GLOBAL, const)
WASM_EXTERNTYPE_CPP_CONV(table, Table, , TABLE, )
WASM_EXTERNTYPE_CPP_CONV(table, Table, , TABLE, const)
WASM_EXTERNTYPE_CPP_CONV(memory, Memory, , MEMORY, )
WASM_EXTERNTYPE_CPP_CONV(memory, Memory, , MEMORY, const)

// <<<<<<<< wasm::Extern functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> wasm::Instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void wasm::Instance::destroy() {
  static_cast<wasm_instance_t *>(this)->~wasm_ref_base_t<wasm_instance_t>();
}

wasm::own<wasm::Instance>
wasm::Instance::make(wasm::Store *, const wasm::Module *,
                     const wasm::vec<wasm::Extern *> &, own<wasm::Trap> *) {
  // TODO: implement this
  return wasm::own<wasm::Instance>();
}

wasm::own<wasm::Instance> wasm::Instance::copy() const {
  return static_cast<const wasm_instance_t *>(this)->copy_ref();
}

wasm::ownvec<wasm::Extern> wasm::Instance::exports() const {
  // TODO: implement this
  return wasm::ownvec<wasm::Extern>::make_uninitialized();
}

// <<<<<<<< wasm::Instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
