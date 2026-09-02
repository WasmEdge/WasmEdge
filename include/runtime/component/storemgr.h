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

/// The component model store. It holds the named component instances, the
/// host component functions, and the component definitions. The instances
/// are owned elsewhere and outlive their registration.
class StoreManager {
public:
  StoreManager() = default;

  /// Register a named component instance. A re-registration shadows the
  /// previous instance, which is the wast-runner semantics.
  void registerInstance(const Instance::ComponentInstance *CompInst) {
    std::unique_lock Lock(Mutex);
    const std::string Name(CompInst->getComponentName());
    NamedInst.insert_or_assign(Name, CompInst);
    if (auto Versioned = parseVersion(Name)) {
      auto &Newest = NewestInst[Versioned->first];
      if (Newest.second == nullptr || Newest.first < Versioned->second) {
        Newest = std::make_pair(Versioned->second, CompInst);
      }
    }
  }

  /// Find a component instance by name. A versioned interface name also
  /// resolves to the newest registered instance of a compatible version:
  /// the same major line (the same minor line below 1.0) at or above the
  /// requested version.
  const Instance::ComponentInstance *findInstance(std::string_view Name) const {
    std::shared_lock Lock(Mutex);
    if (auto Iter = NamedInst.find(Name); likely(Iter != NamedInst.cend())) {
      return Iter->second;
    }
    if (auto Versioned = parseVersion(Name)) {
      if (auto Iter = NewestInst.find(Versioned->first);
          Iter != NewestInst.cend() &&
          !(Iter->second.first < Versioned->second)) {
        return Iter->second.second;
      }
    }
    return nullptr;
  }

  /// Register a named host component function, for a test harness or an
  /// embedder.
  Expect<void> registerFunction(std::string_view Name,
                                Instance::ComponentFunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    if (NamedFunc.find(Name) != NamedFunc.cend()) {
      return Unexpect(ErrCode::Value::ComponentRegistrationConflict);
    }
    NamedFunc.emplace(std::string(Name), Func);
    return {};
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

  /// Register a named component definition. It supplies the component-sort
  /// imports of a root instantiation, for a test harness or an embedder.
  Expect<void> registerDefinition(std::string_view Name,
                                  const AST::Component::Component *Comp) {
    std::unique_lock Lock(Mutex);
    if (NamedDef.find(Name) != NamedDef.cend()) {
      return Unexpect(ErrCode::Value::ComponentRegistrationConflict);
    }
    NamedDef.emplace(std::string(Name), Comp);
    return {};
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
    NamedInst.clear();
    NewestInst.clear();
    NamedFunc.clear();
    NamedDef.clear();
  }

private:
  /// A semantic version; a pre-release sorts below the release it precedes.
  struct Version {
    uint64_t Major = 0;
    uint64_t Minor = 0;
    uint64_t Patch = 0;
    std::string PreRelease;
    bool operator<(const Version &Other) const noexcept {
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

  /// Split `pkg/iface@major.minor.patch[-pre][+build]` into the key of its
  /// compatible line and the version.
  std::optional<std::pair<std::string, Version>>
  parseVersion(std::string_view Name) const noexcept {
    const auto At = Name.rfind('@');
    if (At == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view Rest = Name.substr(At + 1);
    Version Ver;
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
    } else {
      Key += "0." + std::to_string(Ver.Minor);
    }
    return std::make_pair(std::move(Key), std::move(Ver));
  }

  /// \name Mutex for thread-safe.
  mutable std::shared_mutex Mutex;

  /// \name Data of store manager.
  /// @{
  std::map<std::string, const Instance::ComponentInstance *, std::less<>>
      NamedInst;
  /// The newest instance of each compatible version line.
  std::map<std::string, std::pair<Version, const Instance::ComponentInstance *>,
           std::less<>>
      NewestInst;
  std::map<std::string, Instance::ComponentFunctionInstance *, std::less<>>
      NamedFunc;
  std::map<std::string, const AST::Component::Component *, std::less<>>
      NamedDef;
  /// @}
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
