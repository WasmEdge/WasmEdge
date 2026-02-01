// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "common/hash.h"
#include "common/version.h"
#include "loader/shared_library.h"
#include "po/argument_parser.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/module.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#define EXPORT_GET_DESCRIPTOR(Descriptor)                                      \
  extern "C" WASMEDGE_EXPORT decltype(&Descriptor) GetDescriptor();            \
  extern "C" WASMEDGE_EXPORT decltype(&Descriptor) GetDescriptor() {           \
    return &Descriptor;                                                        \
  }

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

class PluginComponent {
public:
  PluginComponent(const PluginComponent &) = delete;
  PluginComponent &operator=(const PluginComponent &) = delete;
  PluginComponent(PluginComponent &&) noexcept = default;
  PluginComponent &operator=(PluginComponent &&) noexcept = default;

  struct ComponentDescriptor {
    const char *Name;
    const char *Description;
    Runtime::Instance::ComponentInstance *(*Create)(
        const ComponentDescriptor *) noexcept;
  };

  constexpr PluginComponent() noexcept {}

  constexpr const char *name() const noexcept {
    assuming(Desc);
    return Desc->Name;
  }

  constexpr const char *description() const noexcept {
    assuming(Desc);
    return Desc->Description;
  }

  std::unique_ptr<Runtime::Instance::ComponentInstance>
  create() const noexcept {
    assuming(Desc);
    return std::unique_ptr<Runtime::Instance::ComponentInstance>(
        Desc->Create(Desc));
  }

private:
  const ComponentDescriptor *Desc = nullptr;

  constexpr explicit PluginComponent(const ComponentDescriptor *D) noexcept
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
    size_t ComponentCount = 0;
    PluginComponent::ComponentDescriptor *ComponentDescriptions = nullptr;
    void (*AddOptions)(const PluginDescriptor *D,
                       PO::ArgumentParser &Parser) noexcept;
  };

  // Const value of current plugin API version.
  static inline constexpr const uint32_t CurrentAPIVersion [[maybe_unused]] =
      kPluginCurrentAPIVersion;

  // Static function to load plugins from default paths.
  WASMEDGE_EXPORT static void loadFromDefaultPaths() noexcept;

  // Static function to get default plugin paths.
  static std::vector<std::filesystem::path> getDefaultPluginPaths() noexcept;

  // Static function to load all plugins from given path.
  WASMEDGE_EXPORT static bool load(const std::filesystem::path &Path) noexcept;

  // Static function to register plugin with descriptor.
  static bool registerPlugin(const PluginDescriptor *Desc) noexcept;

  // Static function to add plugin options from arguments.
  static void addPluginOptions(PO::ArgumentParser &Parser) noexcept;

  // Static function to find loaded plugin by name.
  WASMEDGE_EXPORT static const Plugin *find(std::string_view Name) noexcept;

  // Static function to find loaded plugin by path.
  WASMEDGE_EXPORT static const Plugin *
  findByPath(const std::filesystem::path &Path) noexcept;

  // Static function to list loaded plugins.
  static Span<const Plugin> plugins() noexcept;

  // Static function to unload a plugin by name.
  // Returns true if the plugin was found and unloaded successfully.
  WASMEDGE_EXPORT static bool unload(std::string_view Name) noexcept;

  // Static function to unload a plugin by path.
  // Returns true if the plugin was found and unloaded successfully.
  WASMEDGE_EXPORT static bool
  unloadByPath(const std::filesystem::path &Path) noexcept;

  // Static function to reload a plugin by name.
  // This will unload the current version and load the new version from the
  // same path. Returns true if the plugin was reloaded successfully.
  WASMEDGE_EXPORT static bool reload(std::string_view Name) noexcept;

  // Static function to reload a plugin by path.
  // Returns true if the plugin was reloaded successfully.
  WASMEDGE_EXPORT static bool
  reloadByPath(const std::filesystem::path &Path) noexcept;

  // Static function to check if a plugin is loaded.
  WASMEDGE_EXPORT static bool isLoaded(std::string_view Name) noexcept;

  // Static function to get the number of loaded plugins.
  static size_t count() noexcept;

  Plugin(const Plugin &) = delete;
  Plugin &operator=(const Plugin &) = delete;
  Plugin(Plugin &&) noexcept = default;
  Plugin &operator=(Plugin &&) noexcept = default;

  Plugin() noexcept = default;
  explicit Plugin(const PluginDescriptor *D) noexcept;

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
    return ModuleRegistry;
  }
  Span<const PluginComponent> components() const noexcept {
    assuming(Desc);
    return ComponentRegistry;
  }

  WASMEDGE_EXPORT const PluginModule *
  findModule(std::string_view Name) const noexcept;

  WASMEDGE_EXPORT const PluginComponent *
  findComponent(std::string_view Name) const noexcept;

  std::filesystem::path path() const noexcept { return Path; }

  // Get the last modification time of the plugin file.
  std::filesystem::file_time_type lastModified() const noexcept {
    return LastModified;
  }

  // Get the number of times this plugin has been loaded.
  uint64_t loadCount() const noexcept { return LoadCount; }

  // Check if the plugin file has been modified since it was loaded.
  bool hasChanged() const noexcept;

private:
  static std::mutex Mutex;
  static std::vector<Plugin> PluginRegistry;
  static std::unordered_map<std::string_view, std::size_t, Hash::Hash>
      PluginNameLookup;

  // Static function to load plugin from file. Thread-safe.
  static bool loadFile(const std::filesystem::path &Path) noexcept;

  // Static function to register built-in plugins. Thread-safe.
  static void registerBuiltInPlugins() noexcept;

  // Plugin contents.
  std::filesystem::path Path;
  std::filesystem::file_time_type LastModified;
  uint64_t LoadCount = 0;
  const PluginDescriptor *Desc = nullptr;
  std::shared_ptr<Loader::SharedLibrary> Lib;
  std::vector<PluginModule> ModuleRegistry;
  std::vector<PluginComponent> ComponentRegistry;
  std::unordered_map<std::string_view, std::size_t, Hash::Hash>
      ModuleNameLookup;
  std::unordered_map<std::string_view, std::size_t, Hash::Hash>
      ComponentNameLookup;
};

} // namespace Plugin
} // namespace WasmEdge
