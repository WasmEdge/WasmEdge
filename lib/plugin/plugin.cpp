// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "plugin/plugin.h"
#include "common/errcode.h"
#include "common/version.h"
#include "wasmedge/wasmedge.h"
#include <type_traits>
#include <variant>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dlfcn.h>
#include <pwd.h>
#include <unistd.h>
#elif WASMEDGE_OS_WINDOWS
#include <windows.h>

#include <KnownFolders.h>
#include <ShlObj.h>
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
      return WasmEdge_String{.Length = Length, .Buf = Buf};
    }
    return WasmEdge_String{.Length = 0, .Buf = nullptr};
  }
};
} // namespace PO

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

void IncreaseNiftyCounter() noexcept {
  if (NiftyCounter++ == 0) {
    new (&PluginRegistoryStorage) std::vector<Plugin>();
    new (&PluginNameLookupStorage)
        std::unordered_map<std::string_view, std::size_t>();
  }
}

void DecreaseNiftyCounter() noexcept {
  if (--NiftyCounter == 0) {
    reinterpret_cast<std::vector<Plugin> &>(PluginRegistoryStorage)
        .~vector<Plugin>();
    reinterpret_cast<std::unordered_map<std::string_view, std::size_t> &>(
        PluginNameLookupStorage)
        .~unordered_map<std::string_view, std::size_t>();
  }
}

class CAPIPluginRegister {
public:
  CAPIPluginRegister(const CAPIPluginRegister &) = delete;
  CAPIPluginRegister &operator=(const CAPIPluginRegister &) = delete;

  CAPIPluginRegister(const WasmEdge_PluginDescriptor *Desc) noexcept {
    IncreaseNiftyCounter();

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

    Plugin::registerPlugin(&Descriptor);
  }
  ~CAPIPluginRegister() noexcept { DecreaseNiftyCounter(); }

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
};
std::unordered_map<const PluginModule::ModuleDescriptor *,
                   const WasmEdge_ModuleDescriptor *>
    CAPIPluginRegister::DescriptionLookup;

std::vector<std::unique_ptr<CAPIPluginRegister>> CAPIPluginRegisters;

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

  // Plugin directory for the WasmEdge installation.
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
  Dl_info DLInfo;
  int Status =
      dladdr(reinterpret_cast<void *>(Plugin::getDefaultPluginPaths), &DLInfo);
  if (Status != 0) {
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
  }
#elif WASMEDGE_OS_WINDOWS
  // FIXME: Use the `dladdr`.
  // Global plugin directory.
  Result.push_back(std::filesystem::u8path(kGlobalPluginDir));
  // Local home plugin directory.
  std::filesystem::path Home;
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
  Result.push_back(Home / std::filesystem::u8path(".wasmedge"sv) /
                   std::filesystem::u8path("plugin"sv));
#endif

  return Result;
}

bool Plugin::load(const std::filesystem::path &Path) noexcept {
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

bool Plugin::loadFile(const std::filesystem::path &Path) noexcept {
  const auto Index = PluginRegistory.size();

  auto Lib = std::make_shared<Loader::SharedLibrary>();
  if (auto Res = Lib->load(Path); unlikely(!Res)) {
    return false;
  }

  if (PluginRegistory.size() != Index + 1) {
    // Check C interface
    if (auto GetDescriptor = Lib->get<decltype(WasmEdge_Plugin_GetDescriptor)>(
            "WasmEdge_Plugin_GetDescriptor");
        unlikely(!GetDescriptor)) {
      return false;
    } else if (const auto *Descriptor = GetDescriptor();
               unlikely(!Descriptor)) {
      return false;
    } else {
      CAPIPluginRegisters.push_back(
          std::make_unique<CAPIPluginRegister>(Descriptor));
    }
  }

  auto &Plugin = PluginRegistory.back();
  Plugin.Path = Path;
  Plugin.Lib = std::move(Lib);
  return true;
}

void Plugin::addPluginOptions(PO::ArgumentParser &Parser) noexcept {
  for (const auto &Plugin : PluginRegistory) {
    if (Plugin.Desc->AddOptions) {
      Plugin.Desc->AddOptions(Plugin.Desc, Parser);
    }
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

Span<const Plugin> Plugin::plugins() noexcept { return PluginRegistory; }

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
  IncreaseNiftyCounter();
  Plugin::registerPlugin(Desc);
}

[[gnu::visibility("default")]] PluginRegister::~PluginRegister() noexcept {
  DecreaseNiftyCounter();
}

} // namespace Plugin
} // namespace WasmEdge
