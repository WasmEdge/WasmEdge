// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/storemgr.h - Component Store Manager ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of the component model Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "common/errcode.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/component/function.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Component {

/// A semantic version; a pre-release sorts below the release it precedes.
struct SemVer {
  uint64_t Major = 0;
  uint64_t Minor = 0;
  uint64_t Patch = 0;
  std::string PreRelease;
  bool operator<(const SemVer &Other) const noexcept {
    if (std::tie(Major, Minor, Patch) !=
        std::tie(Other.Major, Other.Minor, Other.Patch)) {
      return std::tie(Major, Minor, Patch) <
             std::tie(Other.Major, Other.Minor, Other.Patch);
    }
    if (PreRelease.empty() != Other.PreRelease.empty()) {
      return !PreRelease.empty();
    }
    return PreRelease < Other.PreRelease;
  }
};

/// The component model store of named instances, host functions, and
/// definitions; it owns none of them.
class StoreManager {
public:
  StoreManager() = default;
  ~StoreManager() noexcept { reset(); }

  /// Register a named component instance.
  Expect<void> registerInstance(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    const std::string Name(CompInst->getComponentName());
    if (NamedInst.find(Name) != NamedInst.cend()) {
      return Unexpect(ErrCode::Value::ComponentRegistrationConflict);
    }
    NamedInst.emplace(Name, CompInst);
    const_cast<Instance::ComponentInstance *>(CompInst)->linkStore(
        this, Name,
        [](const Instance::ComponentInstance::LinkedStoreKey &Key,
           const Instance::ComponentInstance *) {
          std::unique_lock CallbackLock(Key.first->Mutex);
          Key.first->removeInstance(Key.second);
        });
    return {};
  }

  /// Unregister a named component instance.
  Expect<void> unregisterInstance(std::string_view Name) {
    std::unique_lock Lock(Mutex);
    auto Iter = NamedInst.find(Name);
    if (Iter == NamedInst.cend()) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    const_cast<Instance::ComponentInstance *>(Iter->second)
        ->unlinkStore(this, Name);
    removeInstance(Name);
    return {};
  }

  /// Whether exactly Name is registered.
  bool hasInstance(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    return NamedInst.find(Name) != NamedInst.cend();
  }

  /// Find a component instance by name, or the newest compatible version.
  const Instance::ComponentInstance *findInstance(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedInst.find(Name); likely(Iter != NamedInst.cend())) {
      return Iter->second;
    }
    // The names of one interface are neighbours: the newest of the version
    // line that is no older than the one asked for.
    const Instance::ComponentInstance *Newest = nullptr;
    if (auto Versioned = parseVersion(Name)) {
      const std::string_view Prefix = Name.substr(0, Name.rfind('@') + 1);
      for (auto Iter = NamedInst.lower_bound(Prefix);
           Iter != NamedInst.cend() &&
           std::string_view(Iter->first).substr(0, Prefix.size()) == Prefix;
           ++Iter) {
        if (auto Other = parseVersion(Iter->first);
            Other.has_value() && Other->first == Versioned->first &&
            (Newest == nullptr ? !(Other->second < Versioned->second)
                               : Versioned->second < Other->second)) {
          Newest = Iter->second;
          Versioned->second = std::move(Other->second);
        }
      }
    }
    return Newest;
  }

  /// Register a named host component function; the caller keeps it alive
  /// while it is registered.
  Expect<void> registerFunction(std::string_view Name,
                                Instance::ComponentFunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    if (NamedFunc.find(Name) != NamedFunc.cend()) {
      return Unexpect(ErrCode::Value::ComponentRegistrationConflict);
    }
    NamedFunc.emplace(std::string(Name), Func);
    return {};
  }

  /// Unregister a named host component function.
  void unregisterFunction(std::string_view Name) {
    std::unique_lock Lock(Mutex);
    if (auto Iter = NamedFunc.find(Name); Iter != NamedFunc.cend()) {
      NamedFunc.erase(Iter);
    }
  }

  /// Find a host component function by name.
  Instance::ComponentFunctionInstance *
  findFunction(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedFunc.find(Name); Iter != NamedFunc.cend()) {
      return Iter->second;
    }
    return nullptr;
  }

  /// Register a named component definition; the caller keeps it alive while
  /// it is registered.
  Expect<void> registerDefinition(std::string_view Name,
                                  const AST::Component::Component *Comp) {
    std::unique_lock Lock(Mutex);
    if (NamedDef.find(Name) != NamedDef.cend()) {
      return Unexpect(ErrCode::Value::ComponentRegistrationConflict);
    }
    NamedDef.emplace(std::string(Name), Comp);
    return {};
  }

  /// Unregister a named component definition.
  void unregisterDefinition(std::string_view Name) {
    std::unique_lock Lock(Mutex);
    if (auto Iter = NamedDef.find(Name); Iter != NamedDef.cend()) {
      NamedDef.erase(Iter);
    }
  }

  /// Find a component definition by name.
  const AST::Component::Component *findDefinition(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedDef.find(Name); Iter != NamedDef.cend()) {
      return Iter->second;
    }
    return nullptr;
  }

  /// Drop every registration.
  void reset() noexcept {
    std::unique_lock Lock(Mutex);
    for (auto &&[Name, CompInst] : NamedInst) {
      const_cast<Instance::ComponentInstance *>(CompInst)->unlinkStore(this,
                                                                       Name);
    }
    NamedInst.clear();
    NamedFunc.clear();
    NamedDef.clear();
  }

private:
  /// Erase the entry of Name.
  void removeInstance(std::string_view Name) {
    if (auto Iter = NamedInst.find(Name); Iter != NamedInst.cend()) {
      NamedInst.erase(Iter);
    }
  }

  /// Split `pkg/iface@x.y.z[-pre][+build]` into its compatible key and version.
  std::optional<std::pair<std::string, SemVer>>
  parseVersion(std::string_view Name) const noexcept {
    const auto At = Name.rfind('@');
    if (At == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view Rest = Name.substr(At + 1);
    SemVer Ver;
    uint64_t *Parts[] = {&Ver.Major, &Ver.Minor, &Ver.Patch};
    for (uint32_t I = 0; I < 3; ++I) {
      size_t Digits = 0;
      while (Digits < Rest.size() && Rest[Digits] >= '0' &&
             Rest[Digits] <= '9') {
        *Parts[I] = *Parts[I] * 10 + static_cast<uint64_t>(Rest[Digits] - '0');
        ++Digits;
      }
      if (Digits == 0) {
        return std::nullopt;
      }
      Rest.remove_prefix(Digits);
      if (I < 2) {
        if (Rest.empty() || Rest[0] != '.') {
          return std::nullopt;
        }
        Rest.remove_prefix(1);
      }
    }
    if (!Rest.empty() && Rest[0] == '-') {
      const auto Build = Rest.find('+');
      Ver.PreRelease = std::string(Rest.substr(
          1, Build == std::string_view::npos ? std::string_view::npos
                                             : Build - 1));
    } else if (!Rest.empty() && Rest[0] != '+') {
      return std::nullopt;
    }
    std::string Key(Name.substr(0, At + 1));
    if (Ver.Major > 0) {
      Key += std::to_string(Ver.Major);
    } else if (Ver.Minor > 0) {
      Key += "0." + std::to_string(Ver.Minor);
    } else {
      Key += "0.0." + std::to_string(Ver.Patch);
    }
    return std::make_pair(std::move(Key), std::move(Ver));
  }

  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  /// \name Data of store manager.
  /// @{
  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedInst;
  std::map<std::string, Instance::ComponentFunctionInstance *, std::less<>>
      NamedFunc;
  std::map<std::string, const AST::Component::Component *, std::less<>>
      NamedDef;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
