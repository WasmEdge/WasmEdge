// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

// >>>>>>>> WasmEdge compat ABI shims >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
// Several C API functions changed their signatures across releases. Each such
// change pins the previous ABI under its historical version node (bound with
// `.symver`) while `wasmedge.cpp` keeps the new default version, so binaries
// built against the old library keep resolving the old symbol.
//
//   - 0.16 -> 0.17: five functions changed from the by-value `WasmEdge_Limit`
//     struct to the opaque `WasmEdge_LimitContext` handle. Old ABI pinned at
//     `@WASMEDGE_0.16`, new default `@@WASMEDGE_0.17`.
//   - 0.17 -> 0.18: the four `WasmEdge_ModuleInstanceAdd{Function,Table,Memory,
//     Global}` functions changed their return type from `void` to
//     `WasmEdge_Result`. Old ABI pinned at `@WASMEDGE_0.17`, new default
//     `@@WASMEDGE_0.18`.
//
// The shims are frozen copies of the previous-release implementations so the
// old ABIs stay pinned bit-for-bit.
//
// Compiled only into the shared library on Linux/Android (see CMakeLists.txt).
// Kept in a separate TU because under ThinLTO the module-level `.symver` asm
// would otherwise collide with the in-IR definition of the same public symbol.

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

// Local copies of the `wasmedge.cpp` helpers, kept here to stay self-contained.
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

// Layout-compatible revival of the 0.16 `WasmEdge_Limit` struct. `Min`/`Max`
// are `uint32_t` (Memory64 widened them to 64-bit only in 0.17), keeping the
// by-value ABI bit-for-bit identical to what 0.16 consumers were built against.
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

// The 0.17 -> 0.18 transition changed these four functions from returning
// `void` to returning `WasmEdge_Result`. These shims pin the old `void` ABI:
// they perform the same best-effort add and discard the result, matching the
// 0.17 behavior.
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

// Bind each shim to its historical `@WASMEDGE_0.16` symbol name. The shims need
// default visibility for `.symver` to export the alias; their own
// `WasmEdge_*_Compat_016` names are hidden via the `local:` list in
// `lib/api/libwasmedge.lds` so only the `@WASMEDGE_0.16` aliases are exported.
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
        "WasmEdge_ModuleInstanceAddFunction@WASMEDGE_0.17");
__asm__(".symver WasmEdge_ModuleInstanceAddTable_Compat_017, "
        "WasmEdge_ModuleInstanceAddTable@WASMEDGE_0.17");
__asm__(".symver WasmEdge_ModuleInstanceAddMemory_Compat_017, "
        "WasmEdge_ModuleInstanceAddMemory@WASMEDGE_0.17");
__asm__(".symver WasmEdge_ModuleInstanceAddGlobal_Compat_017, "
        "WasmEdge_ModuleInstanceAddGlobal@WASMEDGE_0.17");

#endif // __ELF__ && (__linux__ || __ANDROID__)

// <<<<<<<< WasmEdge compat ABI shims <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
