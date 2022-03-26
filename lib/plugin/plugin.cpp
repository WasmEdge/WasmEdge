// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "plugin/plugin.h"
#include "common/errcode.h"
#include "common/version.h"
#include <type_traits>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <pwd.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include <windows.h>

#include <KnownFolders.h>
#include <ShlObj.h>
#endif

namespace WasmEdge {
namespace Plugin {

namespace {
static unsigned int NiftyCounter = 0;
static std::aligned_storage_t<sizeof(std::vector<Plugin>),
                              alignof(std::vector<Plugin>)>
    PluginRegistoryStorage;
static std::aligned_storage_t<
    sizeof(std::unordered_map<std::string_view, std::size_t>),
    alignof(std::unordered_map<std::string_view, std::size_t>)>
    PluginNameLookupStorage;
} // namespace

std::vector<Plugin> &Plugin::PluginRegistory =
    reinterpret_cast<std::vector<Plugin> &>(PluginRegistoryStorage);
std::unordered_map<std::string_view, std::size_t> &Plugin::PluginNameLookup =
    reinterpret_cast<std::unordered_map<std::string_view, std::size_t> &>(
        PluginNameLookupStorage);

std::vector<std::filesystem::path> Plugin::getDefaultPluginPaths() noexcept {
  using namespace std::literals::string_view_literals;
  std::vector<std::filesystem::path> Result;
  std::error_code Error;

  // Extra directories from environ variable
  if (const auto ExtraEnv = ::getenv("WASMEDGE_PLUGIN_PATH")) {
    std::string_view ExtraEnvStr = ExtraEnv;
    for (auto Sep = ExtraEnvStr.find(':'); Sep != std::string_view::npos;
         Sep = ExtraEnvStr.find(':')) {
      Result.push_back(std::filesystem::u8path(ExtraEnvStr.substr(0, Sep)));
      const auto Next = ExtraEnvStr.find_first_not_of(':', Sep);
      ExtraEnvStr = ExtraEnvStr.substr(Next);
    }
    Result.push_back(std::filesystem::u8path(ExtraEnvStr));
  }

  // Global plugin directory
  Result.push_back(std::filesystem::u8path(kGlobalPluginDir));

  // Local home plugin directory
  std::filesystem::path Home;
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  if (const auto HomeEnv = ::getenv("HOME")) {
    Home = std::filesystem::u8path(HomeEnv);
  } else {
    const auto Passwd = ::getpwuid(::getuid());
    Home = std::filesystem::u8path(Passwd->pw_dir);
  }
#elif WASMEDGE_OS_WINDOWS
  if (const auto HomeEnv = ::getenv("USERPROFILE")) {
    Home = std::filesystem::u8path(HomeEnv);
  } else {
    wchar_t *Path = nullptr;
    if (HRESULT Res =
            ::SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &Path);
        SUCCEEDED(Res)) {
      Home = std::filesystem::path(Path);
      ::CoTaskMemFree(Path);
    }
  }
#endif
  Result.push_back(Home / std::filesystem::u8path(".wasmedge"sv) /
                   std::filesystem::u8path("plugin"sv));

  return Result;
}

bool Plugin::load(const std::filesystem::path &Path) noexcept {
#if WASMEDGE_OS_LINUX
  const auto Extension = ".so"sv;
#elif WASMEDGE_OS_MACOS
  const auto Extension = ".dylib"sv;
#elif WASMEDGE_OS_WINDOWS
  const auto Extension = ".dll"sv;
#endif
  std::error_code Error;
  auto Status = std::filesystem::status(Path, Error);
  if (likely(!Error)) {
    if (std::filesystem::is_directory(Status)) {

      bool Result = false;
      for (const auto &Entry : std::filesystem::recursive_directory_iterator(
               Path, std::filesystem::directory_options::skip_permission_denied,
               Error)) {
        const auto &EntryPath = Entry.path();
        if (Entry.is_regular_file(Error) &&
            EntryPath.extension().u8string() == Extension) {
          Result |= loadFile(EntryPath);
        }
      }
      return Result;
    } else if (std::filesystem::is_regular_file(Status) &&
               Path.extension().u8string() == Extension) {
      return loadFile(Path);
    }
  }
  return false;
}

bool Plugin::loadFile(const std::filesystem::path &Path) noexcept {
  const auto Index = PluginRegistory.size();

  auto Lib = std::make_shared<Loader::SharedLibrary>();
  if (auto Res = Lib->load(Path); unlikely(!Res)) {
    return false;
  }

  if (PluginRegistory.size() != Index + 1) {
    return false;
  }

  auto &Plugin = PluginRegistory.back();
  Plugin.Path = Path;
  Plugin.Lib = std::move(Lib);
  return true;
}

void Plugin::addPluginOptions(PO::ArgumentParser &Parser) noexcept {
  for (const auto &Plugin : PluginRegistory) {
    Plugin.Desc->AddOptions(Parser);
  }
}

const Plugin *Plugin::find(std::string_view Name) noexcept {
  if (NiftyCounter != 0) {
    if (auto Iter = PluginNameLookup.find(Name);
        Iter != PluginNameLookup.end()) {
      return std::addressof(PluginRegistory[Iter->second]);
    }
  }
  return nullptr;
}

[[gnu::visibility("default")]] void
Plugin::registerPlugin(const PluginDescriptor *Desc) noexcept {
  assuming(NiftyCounter != 0);
  if (Desc->APIVersion != CurrentAPIVersion) {
    return;
  }

  const auto Index = PluginRegistory.size();
  PluginRegistory.push_back(Plugin(Desc));
  PluginNameLookup.emplace(Desc->Name, Index);

  return;
}

Plugin::Plugin(const PluginDescriptor *D) noexcept : Desc(D) {
  for (const auto &ModuleDesc : Span<const PluginModule::ModuleDescriptor>(
           D->ModuleDescriptions, D->ModuleCount)) {
    const auto Index = ModuleRegistory.size();
    ModuleRegistory.push_back(PluginModule(&ModuleDesc));
    ModuleNameLookup.emplace(ModuleDesc.Name, Index);
  }
}

const PluginModule *Plugin::findModule(std::string_view Name) const noexcept {
  if (auto Iter = ModuleNameLookup.find(Name); Iter != ModuleNameLookup.end()) {
    return std::addressof(ModuleRegistory[Iter->second]);
  }
  return nullptr;
}

[[gnu::visibility("default")]] PluginRegister::PluginRegister(
    const Plugin::PluginDescriptor *Desc) noexcept {
  if (NiftyCounter++ == 0) {
    new (&PluginRegistoryStorage) std::vector<Plugin>();
    new (&PluginNameLookupStorage)
        std::unordered_map<std::string_view, std::size_t>();
  }
  Plugin::registerPlugin(Desc);
}

[[gnu::visibility("default")]] PluginRegister::~PluginRegister() noexcept {
  if (--NiftyCounter == 0) {
    reinterpret_cast<std::vector<Plugin> &>(PluginRegistoryStorage)
        .~vector<Plugin>();
    reinterpret_cast<std::unordered_map<std::string_view, std::size_t> &>(
        PluginNameLookupStorage)
        .~unordered_map<std::string_view, std::size_t>();
  }
}

} // namespace Plugin
} // namespace WasmEdge
