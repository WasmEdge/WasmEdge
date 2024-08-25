// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "plugin/plugin.h"
#include "common/errcode.h"
#include "common/version.h"
#include "wasmedge/wasmedge.h"

// BUILTIN-PLUGIN: Headers for built-in plug-ins.
#include "plugin/wasi_logging/module.h"

#include <type_traits>
#include <variant>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#include <pwd.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include "system/winapi.h"

static bool GetFunctionModuleFileName(void *FuncPtr,
                                      std::filesystem::path &Path) {
  WasmEdge::winapi::HMODULE_ Module = nullptr;

  if (!WasmEdge::winapi::GetModuleHandleExW(
          WasmEdge::winapi::GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS_ |
              WasmEdge::winapi::GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT_,
          reinterpret_cast<WasmEdge::winapi::LPCWSTR_>(FuncPtr), &Module)) {
    return false;
  }

  std::vector<wchar_t> Buffer;
  WasmEdge::winapi::DWORD_ CopiedSize;
  do {
    Buffer.resize(Buffer.size() + WasmEdge::winapi::MAX_PATH_);
    CopiedSize = WasmEdge::winapi::GetModuleFileNameW(
        Module, Buffer.data(),
        static_cast<WasmEdge::winapi::DWORD_>(Buffer.size()));
    if (CopiedSize == 0) {
      return false;
    }
  } while (CopiedSize >= Buffer.size());

  Path.assign(std::wstring_view(Buffer.data(), CopiedSize));
  return true;
}
#endif

namespace WasmEdge {

namespace PO {
template <> struct Parser<WasmEdge_String> {
  static cxx20::expected<WasmEdge_String, Error>
  parse(std::string Value) noexcept {
    if (!Value.empty()) {
      const uint32_t Length = static_cast<uint32_t>(Value.size());
      char *Buf = new char[Value.size()];
      std::copy_n(Value.data(), Value.size(), Buf);
      return WasmEdge_String{/* Length */ Length, /* Buf */ Buf};
    }
    return WasmEdge_String{/* Length */ 0, /* Buf */ nullptr};
  }
};
} // namespace PO

namespace Plugin {

namespace {

class CAPIPluginRegister {
public:
  CAPIPluginRegister(const CAPIPluginRegister &) = delete;
  CAPIPluginRegister &operator=(const CAPIPluginRegister &) = delete;

  CAPIPluginRegister(const WasmEdge_PluginDescriptor *Desc) noexcept {
    ModuleDescriptions.resize(Desc->ModuleCount);
    for (size_t I = 0; I < ModuleDescriptions.size(); ++I) {
      ModuleDescriptions[I].Name = Desc->ModuleDescriptions[I].Name;
      ModuleDescriptions[I].Description =
          Desc->ModuleDescriptions[I].Description;
      ModuleDescriptions[I].Create = &createWrapper;
      DescriptionLookup.emplace(&ModuleDescriptions[I],
                                &Desc->ModuleDescriptions[I]);
    }

    Descriptor.Name = Desc->Name;
    Descriptor.Description = Desc->Description;
    Descriptor.APIVersion = Desc->APIVersion;
    Descriptor.Version.Major = Desc->Version.Major;
    Descriptor.Version.Minor = Desc->Version.Minor;
    Descriptor.Version.Patch = Desc->Version.Patch;
    Descriptor.Version.Build = Desc->Version.Build;
    Descriptor.ModuleCount = Desc->ModuleCount;
    Descriptor.ModuleDescriptions = ModuleDescriptions.data();
    Descriptor.AddOptions = &addOptionsWrapper;

    for (size_t I = 0; I < Desc->ProgramOptionCount; ++I) {
      const auto *OptionDesc = &Desc->ProgramOptions[I];
      auto Emplace = [OptionDesc, this](auto InPlaceType, auto *Storage,
                                        auto *DefaultValue) {
        Options.emplace_back(
            std::piecewise_construct, std::tuple{OptionDesc},
            std::tuple{InPlaceType, PO::Description(OptionDesc->Description),
                       Storage, DefaultValue});
      };
      switch (Desc->ProgramOptions[I].Type) {
      case WasmEdge_ProgramOptionType_None:
        break;
      case WasmEdge_ProgramOptionType_Toggle:
        Emplace(std::in_place_type<PO::Option<PO::Toggle *>>,
                static_cast<bool *>(OptionDesc->Storage),
                static_cast<const bool *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Int8:
        Emplace(std::in_place_type<PO::Option<int8_t *>>,
                static_cast<int8_t *>(OptionDesc->Storage),
                static_cast<const int8_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Int16:
        Emplace(std::in_place_type<PO::Option<int16_t *>>,
                static_cast<int16_t *>(OptionDesc->Storage),
                static_cast<const int16_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Int32:
        Emplace(std::in_place_type<PO::Option<int32_t *>>,
                static_cast<int32_t *>(OptionDesc->Storage),
                static_cast<const int32_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Int64:
        Emplace(std::in_place_type<PO::Option<int64_t *>>,
                static_cast<int64_t *>(OptionDesc->Storage),
                static_cast<const int64_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_UInt8:
        Emplace(std::in_place_type<PO::Option<uint8_t *>>,
                static_cast<uint8_t *>(OptionDesc->Storage),
                static_cast<const uint8_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_UInt16:
        Emplace(std::in_place_type<PO::Option<uint16_t *>>,
                static_cast<uint16_t *>(OptionDesc->Storage),
                static_cast<const uint16_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_UInt32:
        Emplace(std::in_place_type<PO::Option<uint32_t *>>,
                static_cast<uint32_t *>(OptionDesc->Storage),
                static_cast<const uint32_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_UInt64:
        Emplace(std::in_place_type<PO::Option<uint64_t *>>,
                static_cast<uint64_t *>(OptionDesc->Storage),
                static_cast<const uint64_t *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Float:
        Emplace(std::in_place_type<PO::Option<float *>>,
                static_cast<float *>(OptionDesc->Storage),
                static_cast<const float *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_Double:
        Emplace(std::in_place_type<PO::Option<double *>>,
                static_cast<double *>(OptionDesc->Storage),
                static_cast<const double *>(OptionDesc->DefaultValue));
        break;
      case WasmEdge_ProgramOptionType_String:
        Emplace(std::in_place_type<PO::Option<WasmEdge_String *>>,
                static_cast<WasmEdge_String *>(OptionDesc->Storage),
                static_cast<const WasmEdge_String *>(OptionDesc->DefaultValue));
        break;
      }
    }

    Result = Plugin::registerPlugin(&Descriptor);
  }
  bool result() const noexcept { return Result; }

private:
  static Runtime::Instance::ModuleInstance *
  createWrapper(const PluginModule::ModuleDescriptor *Descriptor) noexcept {
    static_assert(std::is_standard_layout_v<CAPIPluginRegister>);
    if (auto Iter = DescriptionLookup.find(Descriptor);
        unlikely(Iter == DescriptionLookup.end())) {
      return nullptr;
    } else {
      return reinterpret_cast<Runtime::Instance::ModuleInstance *>(
          Iter->second->Create(Iter->second));
    }
  }
  static void addOptionsWrapper(const Plugin::PluginDescriptor *Descriptor,
                                PO::ArgumentParser &Parser
                                [[maybe_unused]]) noexcept {
    const CAPIPluginRegister *This =
        reinterpret_cast<const CAPIPluginRegister *>(
            reinterpret_cast<uintptr_t>(Descriptor) -
            offsetof(CAPIPluginRegister, Descriptor));
    for (auto &Option : This->Options) {
      std::visit(
          [&Option, &Parser](auto &POOption) {
            Parser.add_option(Option.first->Name, POOption);
          },
          Option.second);
    }
  }

  Plugin::PluginDescriptor Descriptor;
  mutable std::vector<std::pair<
      const WasmEdge_ProgramOption *,
      std::variant<PO::Option<PO::Toggle *>, PO::Option<int8_t *>,
                   PO::Option<int16_t *>, PO::Option<int32_t *>,
                   PO::Option<int64_t *>, PO::Option<uint8_t *>,
                   PO::Option<uint16_t *>, PO::Option<uint32_t *>,
                   PO::Option<uint64_t *>, PO::Option<float *>,
                   PO::Option<double *>, PO::Option<WasmEdge_String *>>>>
      Options;
  std::vector<PluginModule::ModuleDescriptor> ModuleDescriptions;
  static std::unordered_map<const PluginModule::ModuleDescriptor *,
                            const WasmEdge_ModuleDescriptor *>
      DescriptionLookup;

  bool Result = false;
};
std::unordered_map<const PluginModule::ModuleDescriptor *,
                   const WasmEdge_ModuleDescriptor *>
    CAPIPluginRegister::DescriptionLookup;

std::vector<std::unique_ptr<CAPIPluginRegister>> CAPIPluginRegisters;

} // namespace

std::mutex WasmEdge::Plugin::Plugin::Mutex;
std::vector<Plugin> WasmEdge::Plugin::Plugin::PluginRegistry;
std::unordered_map<std::string_view, std::size_t>
    WasmEdge::Plugin::Plugin::PluginNameLookup;

void Plugin::loadFromDefaultPaths() noexcept {
  registerBuiltInPlugins();
  for (const auto &Path : Plugin::Plugin::getDefaultPluginPaths()) {
    Plugin::Plugin::load(Path);
  }
}

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

  // Plugin directory for the WasmEdge installation.
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  Dl_info DLInfo;
  int Status =
      dladdr(reinterpret_cast<void *>(Plugin::getDefaultPluginPaths), &DLInfo);
  if (Status != 0) {
    if (DLInfo.dli_fname == nullptr) {
      spdlog::error(
          "Address matched to a shared object but not to any symbol "sv
          "within the object. dli_fname is null."sv);
      return std::vector<std::filesystem::path>();
    }
    auto LibPath = std::filesystem::u8path(DLInfo.dli_fname)
                       .parent_path()
                       .lexically_normal();
    const auto UsrStr = "/usr"sv;
    const auto LibStr = "/lib"sv;
    const auto &PathStr = LibPath.native();
    if ((PathStr.size() >= UsrStr.size() &&
         std::equal(UsrStr.begin(), UsrStr.end(), PathStr.begin())) ||
        (PathStr.size() >= LibStr.size() &&
         std::equal(LibStr.begin(), LibStr.end(), PathStr.begin()))) {
      // The installation path of the WasmEdge library is under "/usr".
      // Plug-in path will be in "LIB_PATH/wasmedge".
      // If the installation path is under "/usr/lib" or "/usr/lib64", the
      // traced library path will be "/lib" or "/lib64".
      Result.push_back(LibPath / std::filesystem::u8path("wasmedge"sv));
    } else {
      // The installation path of the WasmEdge library is not under "/usr", such
      // as "$HOME/.wasmedge". Plug-in path will be in "LIB_PATH/../plugin".
      Result.push_back(LibPath / std::filesystem::u8path(".."sv) /
                       std::filesystem::u8path("plugin"sv));
    }
  } else {
    spdlog::error(ErrCode::Value::NonNullRequired);
    spdlog::error("Address could not be matched to any shared object. "sv
                  "Detailed error information is not available."sv);
    return std::vector<std::filesystem::path>();
  }
#elif WASMEDGE_OS_WINDOWS
  // Global plugin directory.
  if (std::filesystem::path Path; GetFunctionModuleFileName(
          reinterpret_cast<void *>(Plugin::getDefaultPluginPaths), Path)) {
    Result.push_back(Path.parent_path());
  } else {
    spdlog::error("Failed to get the path of the current module."sv);
    return std::vector<std::filesystem::path>();
  }

  // Local home plugin directory.
  std::filesystem::path Home;
  if (const auto HomeEnv = ::getenv("USERPROFILE")) {
    Home = std::filesystem::u8path(HomeEnv);
  } else {
#if NTDDI_VERSION >= NTDDI_VISTA
    wchar_t *Path = nullptr;
    if (winapi::HRESULT_ Res = winapi::SHGetKnownFolderPath(
            winapi::FOLDERID_Profile, 0, nullptr, &Path);
        winapi::SUCCEEDED_(Res)) {
      Home = std::filesystem::path(Path);
      winapi::CoTaskMemFree(Path);
    }
#else
    wchar_t Path[winapi::MAX_PATH_];
    if (winapi::HRESULT_ Res = winapi::SHGetFolderPathW(
            nullptr, winapi::CSIDL_PROFILE_, nullptr, 0, Path);
        winapi::SUCCEEDED_(Res)) {
      Home = std::filesystem::path(Path);
    }
#endif
  }
  Result.push_back(Home / std::filesystem::u8path(".wasmedge"sv) /
                   std::filesystem::u8path("plugin"sv));
#endif

  return Result;
}

WASMEDGE_EXPORT bool Plugin::load(const std::filesystem::path &Path) noexcept {
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
            EntryPath.extension().u8string() == WASMEDGE_LIB_EXTENSION) {
          Result |= loadFile(EntryPath);
        }
      }
      return Result;
    } else if (std::filesystem::is_regular_file(Status) &&
               Path.extension().u8string() == WASMEDGE_LIB_EXTENSION) {
      return loadFile(Path);
    }
  }
  return false;
}

bool Plugin::registerPlugin(const PluginDescriptor *Desc) noexcept {
  if (Desc->APIVersion != CurrentAPIVersion) {
    spdlog::debug(
        "Plugin: API version {} of plugin {} is not match to current {}."sv,
        Desc->APIVersion, Desc->Name, CurrentAPIVersion);
    return false;
  }
  if (PluginNameLookup.find(Desc->Name) != PluginNameLookup.end()) {
    spdlog::debug("Plugin: {} has already loaded."sv, Desc->Name);
    return false;
  }

  const auto Index = PluginRegistry.size();
  PluginRegistry.emplace_back(Desc);
  PluginNameLookup.emplace(Desc->Name, Index);

  return true;
}

void Plugin::addPluginOptions(PO::ArgumentParser &Parser) noexcept {
  for (const auto &Plugin : PluginRegistry) {
    if (Plugin.Desc->AddOptions) {
      Plugin.Desc->AddOptions(Plugin.Desc, Parser);
    }
  }
}

WASMEDGE_EXPORT const Plugin *Plugin::find(std::string_view Name) noexcept {
  if (auto Iter = PluginNameLookup.find(Name); Iter != PluginNameLookup.end()) {
    return std::addressof(PluginRegistry[Iter->second]);
  }
  return nullptr;
}

Span<const Plugin> Plugin::plugins() noexcept { return PluginRegistry; }

bool Plugin::loadFile(const std::filesystem::path &Path) noexcept {
  std::unique_lock Lock(Mutex);
  bool Result = false;
  auto Lib = std::make_shared<Loader::SharedLibrary>();
  if (auto Res = Lib->load(Path); unlikely(!Res)) {
    return false;
  }

  if (auto GetDescriptor =
          Lib->get<Plugin::PluginDescriptor const *()>("GetDescriptor")) {
    Result = Plugin::registerPlugin(GetDescriptor());
  }

  if (!Result) {
    // Check C interface
    if (auto GetDescriptor = Lib->get<decltype(WasmEdge_Plugin_GetDescriptor)>(
            "WasmEdge_Plugin_GetDescriptor");
        unlikely(!GetDescriptor)) {
      return false;
    } else if (const auto *Descriptor = GetDescriptor();
               unlikely(!Descriptor)) {
      return false;
    } else {
      Result =
          CAPIPluginRegisters
              .emplace_back(std::make_unique<CAPIPluginRegister>(Descriptor))
              ->result();
    }
  }

  if (!Result) {
    return false;
  }

  auto &Plugin = PluginRegistry.back();
  Plugin.Path = Path;
  Plugin.Lib = std::move(Lib);
  return true;
}

void Plugin::registerBuiltInPlugins() noexcept {
  std::unique_lock Lock(Mutex);
  // BUILTIN-PLUGIN: Register wasi-logging here. May be refactored in 0.15.0.
  registerPlugin(&Host::WasiLoggingModule::PluginDescriptor);
}

Plugin::Plugin(const PluginDescriptor *D) noexcept : Desc(D) {
  for (const auto &ModuleDesc : Span<const PluginModule::ModuleDescriptor>(
           D->ModuleDescriptions, D->ModuleCount)) {
    const auto Index = ModuleRegistry.size();
    ModuleRegistry.push_back(PluginModule(&ModuleDesc));
    ModuleNameLookup.emplace(ModuleDesc.Name, Index);
  }
  for (const auto &ComponentDesc :
       Span<const PluginComponent::ComponentDescriptor>(
           D->ComponentDescriptions, D->ComponentCount)) {
    const auto Index = ComponentRegistry.size();
    ComponentRegistry.push_back(PluginComponent(&ComponentDesc));
    ComponentNameLookup.emplace(ComponentDesc.Name, Index);
  }
}

WASMEDGE_EXPORT const PluginModule *
Plugin::findModule(std::string_view Name) const noexcept {
  if (auto Iter = ModuleNameLookup.find(Name); Iter != ModuleNameLookup.end()) {
    return std::addressof(ModuleRegistry[Iter->second]);
  }
  return nullptr;
}

WASMEDGE_EXPORT const PluginComponent *
Plugin::findComponent(std::string_view Name) const noexcept {
  if (auto Iter = ComponentNameLookup.find(Name);
      Iter != ComponentNameLookup.end()) {
    return std::addressof(ComponentRegistry[Iter->second]);
  }
  return nullptr;
}
} // namespace Plugin
} // namespace WasmEdge
