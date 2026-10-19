// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

// >>>>>>>> WasmEdge compat ABI shims >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
// Frozen previous-release implementations of the C API functions whose
// signatures changed, bound with `.symver` to the version node each symbol had
// in that release. See .github/scripts/SYMBOL_CHECKING.md.
//
//   - 0.16 -> 0.17: `WasmEdge_Limit` struct -> `WasmEdge_LimitContext`,
//     `@WASMEDGE_0.16` beside the `@@WASMEDGE_0.17` default.
//   - 0.17 -> 0.18: `WasmEdge_ModuleInstanceAdd*` return `void` ->
//     `WasmEdge_Result`, `@WASMEDGE_0.16` beside the `@@WASMEDGE_0.18` default.
//
// A separate TU, so the `.symver` asm does not collide with the in-IR public
// definitions under ThinLTO.

#include "wasmedge/wasmedge.h"

#include "ast/type.h"
#include "runtime/instance/module.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#if defined(__ELF__) && (defined(__linux__) || defined(__ANDROID__))

namespace {
using namespace WasmEdge;

// Copies of the `wasmedge.cpp` helpers.
inline ValType genValType(const WasmEdge_ValType &T) noexcept {
  std::array<uint8_t, 8> R;
  std::copy_n(T.Data, 8, R.begin());
  return ValType(R);
}
inline auto *toTabTypeCxt(AST::TableType *Cxt) noexcept {
  return reinterpret_cast<WasmEdge_TableTypeContext *>(Cxt);
}
inline const auto *
fromTabTypeCxt(const WasmEdge_TableTypeContext *Cxt) noexcept {
  return reinterpret_cast<const AST::TableType *>(Cxt);
}
inline auto *toMemTypeCxt(AST::MemoryType *Cxt) noexcept {
  return reinterpret_cast<WasmEdge_MemoryTypeContext *>(Cxt);
}
inline const auto *
fromMemTypeCxt(const WasmEdge_MemoryTypeContext *Cxt) noexcept {
  return reinterpret_cast<const AST::MemoryType *>(Cxt);
}
inline std::string_view genStrView(const WasmEdge_String S) noexcept {
  return std::string_view(S.Buf, S.Length);
}
inline auto *fromModCxt(WasmEdge_ModuleInstanceContext *Cxt) noexcept {
  return reinterpret_cast<Runtime::Instance::ModuleInstance *>(Cxt);
}
inline auto *fromFuncCxt(WasmEdge_FunctionInstanceContext *Cxt) noexcept {
  return reinterpret_cast<Runtime::Instance::FunctionInstance *>(Cxt);
}
inline auto *fromTabCxt(WasmEdge_TableInstanceContext *Cxt) noexcept {
  return reinterpret_cast<Runtime::Instance::TableInstance *>(Cxt);
}
inline auto *fromMemCxt(WasmEdge_MemoryInstanceContext *Cxt) noexcept {
  return reinterpret_cast<Runtime::Instance::MemoryInstance *>(Cxt);
}
inline auto *fromGlobCxt(WasmEdge_GlobalInstanceContext *Cxt) noexcept {
  return reinterpret_cast<Runtime::Instance::GlobalInstance *>(Cxt);
}
} // namespace

extern "C" {

// The 0.16 `WasmEdge_Limit` layout, before Memory64 widened `Min`/`Max`.
typedef struct WasmEdge_Limit_0_16 {
  bool HasMax;
  bool Shared;
  uint32_t Min;
  uint32_t Max;
} WasmEdge_Limit_0_16;

__attribute__((used, visibility("default"))) bool
WasmEdge_LimitIsEqual_Compat_016(const WasmEdge_Limit_0_16 Lim1,
                                 const WasmEdge_Limit_0_16 Lim2) {
  return Lim1.HasMax == Lim2.HasMax && Lim1.Shared == Lim2.Shared &&
         Lim1.Min == Lim2.Min && Lim1.Max == Lim2.Max;
}

__attribute__((used, visibility("default"))) WasmEdge_TableTypeContext *
WasmEdge_TableTypeCreate_Compat_016(const WasmEdge_ValType RefType,
                                    const WasmEdge_Limit_0_16 Limit) {
  WasmEdge::ValType RT = genValType(RefType);
  if (!RT.isRefType()) {
    return nullptr;
  }
  if (Limit.HasMax) {
    return toTabTypeCxt(new WasmEdge::AST::TableType(RT, Limit.Min, Limit.Max));
  } else {
    return toTabTypeCxt(new WasmEdge::AST::TableType(RT, Limit.Min));
  }
}

__attribute__((used, visibility("default"))) WasmEdge_Limit_0_16
WasmEdge_TableTypeGetLimit_Compat_016(const WasmEdge_TableTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromTabTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit_0_16{/* HasMax */ Lim.hasMax(),
                               /* Shared */ Lim.isShared(),
                               /* Min */ static_cast<uint32_t>(Lim.getMin()),
                               /* Max */ static_cast<uint32_t>(Lim.getMax())};
  }
  return WasmEdge_Limit_0_16{/* HasMax */ false, /* Shared */ false,
                             /* Min */ 0, /* Max */ 0};
}

__attribute__((used, visibility("default"))) WasmEdge_MemoryTypeContext *
WasmEdge_MemoryTypeCreate_Compat_016(const WasmEdge_Limit_0_16 Limit) {
  if (Limit.Shared) {
    return toMemTypeCxt(
        new WasmEdge::AST::MemoryType(Limit.Min, Limit.Max, true));
  } else if (Limit.HasMax) {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min, Limit.Max));
  } else {
    return toMemTypeCxt(new WasmEdge::AST::MemoryType(Limit.Min));
  }
}

__attribute__((used, visibility("default"))) WasmEdge_Limit_0_16
WasmEdge_MemoryTypeGetLimit_Compat_016(const WasmEdge_MemoryTypeContext *Cxt) {
  if (Cxt) {
    const auto &Lim = fromMemTypeCxt(Cxt)->getLimit();
    return WasmEdge_Limit_0_16{/* HasMax */ Lim.hasMax(),
                               /* Shared */ Lim.isShared(),
                               /* Min */ static_cast<uint32_t>(Lim.getMin()),
                               /* Max */ static_cast<uint32_t>(Lim.getMax())};
  }
  return WasmEdge_Limit_0_16{/* HasMax */ false, /* Shared */ false,
                             /* Min */ 0, /* Max */ 0};
}

// The 0.17 `void` versions: add and discard the result.
__attribute__((used, visibility("default"))) void
WasmEdge_ModuleInstanceAddFunction_Compat_017(
    WasmEdge_ModuleInstanceContext *Cxt, const WasmEdge_String Name,
    WasmEdge_FunctionInstanceContext *FuncCxt) {
  if (Cxt && FuncCxt) {
    static_cast<void>(fromModCxt(Cxt)->addHostFunc(
        genStrView(Name), std::unique_ptr<Runtime::Instance::FunctionInstance>(
                              fromFuncCxt(FuncCxt))));
  }
}

__attribute__((used, visibility("default"))) void
WasmEdge_ModuleInstanceAddTable_Compat_017(
    WasmEdge_ModuleInstanceContext *Cxt, const WasmEdge_String Name,
    WasmEdge_TableInstanceContext *TableCxt) {
  if (Cxt && TableCxt) {
    static_cast<void>(fromModCxt(Cxt)->addHostTable(
        genStrView(Name), std::unique_ptr<Runtime::Instance::TableInstance>(
                              fromTabCxt(TableCxt))));
  }
}

__attribute__((used, visibility("default"))) void
WasmEdge_ModuleInstanceAddMemory_Compat_017(
    WasmEdge_ModuleInstanceContext *Cxt, const WasmEdge_String Name,
    WasmEdge_MemoryInstanceContext *MemoryCxt) {
  if (Cxt && MemoryCxt) {
    static_cast<void>(fromModCxt(Cxt)->addHostMemory(
        genStrView(Name), std::unique_ptr<Runtime::Instance::MemoryInstance>(
                              fromMemCxt(MemoryCxt))));
  }
}

__attribute__((used, visibility("default"))) void
WasmEdge_ModuleInstanceAddGlobal_Compat_017(
    WasmEdge_ModuleInstanceContext *Cxt, const WasmEdge_String Name,
    WasmEdge_GlobalInstanceContext *GlobalCxt) {
  if (Cxt && GlobalCxt) {
    static_cast<void>(fromModCxt(Cxt)->addHostGlobal(
        genStrView(Name), std::unique_ptr<Runtime::Instance::GlobalInstance>(
                              fromGlobCxt(GlobalCxt))));
  }
}

} // extern "C"

// The shims need default visibility to export the aliases; their own names are
// hidden by the `local:` list in `lib/api/libwasmedge.lds`.
__asm__(".symver WasmEdge_LimitIsEqual_Compat_016, "
        "WasmEdge_LimitIsEqual@WASMEDGE_0.16");
__asm__(".symver WasmEdge_TableTypeCreate_Compat_016, "
        "WasmEdge_TableTypeCreate@WASMEDGE_0.16");
__asm__(".symver WasmEdge_TableTypeGetLimit_Compat_016, "
        "WasmEdge_TableTypeGetLimit@WASMEDGE_0.16");
__asm__(".symver WasmEdge_MemoryTypeCreate_Compat_016, "
        "WasmEdge_MemoryTypeCreate@WASMEDGE_0.16");
__asm__(".symver WasmEdge_MemoryTypeGetLimit_Compat_016, "
        "WasmEdge_MemoryTypeGetLimit@WASMEDGE_0.16");
__asm__(".symver WasmEdge_ModuleInstanceAddFunction_Compat_017, "
        "WasmEdge_ModuleInstanceAddFunction@WASMEDGE_0.16");
__asm__(".symver WasmEdge_ModuleInstanceAddTable_Compat_017, "
        "WasmEdge_ModuleInstanceAddTable@WASMEDGE_0.16");
__asm__(".symver WasmEdge_ModuleInstanceAddMemory_Compat_017, "
        "WasmEdge_ModuleInstanceAddMemory@WASMEDGE_0.16");
__asm__(".symver WasmEdge_ModuleInstanceAddGlobal_Compat_017, "
        "WasmEdge_ModuleInstanceAddGlobal@WASMEDGE_0.16");

#endif // __ELF__ && (__linux__ || __ANDROID__)

// <<<<<<<< WasmEdge compat ABI shims <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
