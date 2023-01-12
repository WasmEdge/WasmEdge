// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/plugin/plugin.h - Plugin class definition ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Plugin class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/version.h"
#include "loader/shared_library.h"
#include "po/argument_parser.h"
#include "runtime/instance/module.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Plugin {

class PluginModule {
public:
  PluginModule(const PluginModule &) = delete;
  PluginModule &operator=(const PluginModule &) = delete;
  PluginModule(PluginModule &&) noexcept = default;
  PluginModule &operator=(PluginModule &&) noexcept = default;

  struct ModuleDescriptor {
    const char *Name;
    const char *Description;
    Runtime::Instance::ModuleInstance *(*Create)(
        const ModuleDescriptor *) noexcept;
  };

  constexpr PluginModule() noexcept {}

  constexpr const char *name() const noexcept {
    assuming(Desc);
    return Desc->Name;
  }

  constexpr const char *description() const noexcept {
    assuming(Desc);
    return Desc->Description;
  }

  std::unique_ptr<Runtime::Instance::ModuleInstance> create() const noexcept {
    assuming(Desc);
    return std::unique_ptr<Runtime::Instance::ModuleInstance>(
        Desc->Create(Desc));
  }

private:
  const ModuleDescriptor *Desc = nullptr;

  constexpr explicit PluginModule(const ModuleDescriptor *D) noexcept
      : Desc(D) {}
  friend class Plugin;
};

class Plugin {
public:
  struct VersionData {
    uint32_t Major;
    uint32_t Minor;
    uint32_t Patch;
    uint32_t Build;
  };
  struct PluginDescriptor {
    const char *Name;
    const char *Description;
    uint32_t APIVersion;
    VersionData Version;
    size_t ModuleCount;
    PluginModule::ModuleDescriptor *ModuleDescriptions;
    void (*AddOptions)(const PluginDescriptor *D,
                       PO::ArgumentParser &Parser) noexcept;
  };

  static inline constexpr const uint32_t CurrentAPIVersion [[maybe_unused]] =
      kPluginCurrentAPIVersion;
  static std::vector<std::filesystem::path> getDefaultPluginPaths() noexcept;
  static bool load(const std::filesystem::path &Path) noexcept;
  static void addPluginOptions(PO::ArgumentParser &Parser) noexcept;
  static const Plugin *find(std::string_view Name) noexcept;
  static Span<const Plugin> plugins() noexcept;

  Plugin(const Plugin &) = delete;
  Plugin &operator=(const Plugin &) = delete;
  Plugin(Plugin &&) noexcept = default;
  Plugin &operator=(Plugin &&) noexcept = default;

  Plugin() noexcept = default;

  constexpr const char *name() const noexcept {
    assuming(Desc);
    return Desc->Name;
  }

  constexpr const char *description() const noexcept {
    assuming(Desc);
    return Desc->Description;
  }

  constexpr VersionData version() const noexcept {
    assuming(Desc);
    return Desc->Version;
  }

  void registerOptions(PO::ArgumentParser &Parser) const noexcept {
    assuming(Desc);
    if (Desc->AddOptions) {
      Desc->AddOptions(Desc, Parser);
    }
  }

  Span<const PluginModule> modules() const noexcept {
    assuming(Desc);
    return ModuleRegistory;
  }

  const PluginModule *findModule(std::string_view Name) const noexcept;

private:
  static std::vector<Plugin> &PluginRegistory;
  static std::unordered_map<std::string_view, std::size_t> &PluginNameLookup;

  std::filesystem::path Path;
  const PluginDescriptor *Desc = nullptr;
  std::shared_ptr<Loader::SharedLibrary> Lib;
  std::vector<PluginModule> ModuleRegistory;
  std::unordered_map<std::string_view, std::size_t> ModuleNameLookup;

  static bool loadFile(const std::filesystem::path &Path) noexcept;

  explicit Plugin(const PluginDescriptor *D) noexcept;

public:
  static void registerPlugin(const PluginDescriptor *Desc) noexcept;
};

struct PluginRegister {
  PluginRegister(const Plugin::PluginDescriptor *Desc) noexcept;

  ~PluginRegister() noexcept;
};

} // namespace Plugin
} // namespace WasmEdge
