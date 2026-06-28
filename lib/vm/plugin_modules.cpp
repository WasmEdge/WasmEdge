// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "plugin_modules.h"

#include "common/spdlog.h"
#include "plugin/plugin.h"

#include "host/mock/wasi_crypto_module.h"
#include "host/mock/wasi_logging_module.h"
#include "host/mock/wasi_nn_module.h"
#include "host/mock/wasmedge_image_module.h"
#include "host/mock/wasmedge_process_module.h"
#include "host/mock/wasmedge_stablediffusion_module.h"
#include "host/mock/wasmedge_tensorflow_module.h"
#include "host/mock/wasmedge_tensorflowlite_module.h"

#include <algorithm>
#include <array>

namespace WasmEdge {
namespace VM {

namespace {

using namespace std::literals::string_view_literals;

using ModuleFactory = std::unique_ptr<Runtime::Instance::ModuleInstance> (*)();

template <typename T>
std::unique_ptr<Runtime::Instance::ModuleInstance> createMock() {
  return std::make_unique<T>();
}

struct OfficialPluginEntry {
  std::string_view PluginName;
  std::string_view ModuleName;
  ModuleFactory MockFactory;
};

/// One row per official plugin module. Both the load order of the mock-or-real
/// instances and the official plugin name check derive from this table.
constexpr std::array<OfficialPluginEntry, 12> OfficialPluginTable{{
    {"wasi_nn"sv, "wasi_nn"sv, createMock<Host::WasiNNModuleMock>},
    {"wasi_crypto"sv, "wasi_crypto_common"sv,
     createMock<Host::WasiCryptoCommonModuleMock>},
    {"wasi_crypto"sv, "wasi_crypto_asymmetric_common"sv,
     createMock<Host::WasiCryptoAsymmetricCommonModuleMock>},
    {"wasi_crypto"sv, "wasi_crypto_kx"sv,
     createMock<Host::WasiCryptoKxModuleMock>},
    {"wasi_crypto"sv, "wasi_crypto_signatures"sv,
     createMock<Host::WasiCryptoSignaturesModuleMock>},
    {"wasi_crypto"sv, "wasi_crypto_symmetric"sv,
     createMock<Host::WasiCryptoSymmetricModuleMock>},
    {"wasmedge_process"sv, "wasmedge_process"sv,
     createMock<Host::WasmEdgeProcessModuleMock>},
    {"wasi_logging"sv, "wasi:logging/logging"sv,
     createMock<Host::WasiLoggingModuleMock>},
    {"wasmedge_tensorflow"sv, "wasmedge_tensorflow"sv,
     createMock<Host::WasmEdgeTensorflowModuleMock>},
    {"wasmedge_tensorflowlite"sv, "wasmedge_tensorflowlite"sv,
     createMock<Host::WasmEdgeTensorflowLiteModuleMock>},
    {"wasmedge_image"sv, "wasmedge_image"sv,
     createMock<Host::WasmEdgeImageModuleMock>},
    {"wasmedge_stablediffusion"sv, "wasmedge_stablediffusion"sv,
     createMock<Host::WasmEdgeStableDiffusionModuleMock>},
}};

std::unique_ptr<Runtime::Instance::ModuleInstance>
createPluginModule(const OfficialPluginEntry &Entry) {
  if (const auto *Plugin = Plugin::Plugin::find(Entry.PluginName)) {
    if (const auto *Module = Plugin->findModule(Entry.ModuleName)) {
      return Module->create();
    }
  }
  spdlog::debug("Plugin: {} , module name: {} not found. Mock instead."sv,
                Entry.PluginName, Entry.ModuleName);
  return Entry.MockFactory();
}

} // namespace

std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>>
loadOfficialPluginModules() {
  std::vector<std::unique_ptr<Runtime::Instance::ModuleInstance>> Modules;
  Modules.reserve(OfficialPluginTable.size());
  for (const auto &Entry : OfficialPluginTable) {
    Modules.push_back(createPluginModule(Entry));
  }
  return Modules;
}

bool isOfficialPlugin(std::string_view PName) {
  return std::any_of(OfficialPluginTable.begin(), OfficialPluginTable.end(),
                     [PName](const OfficialPluginEntry &Entry) {
                       return Entry.PluginName == PName;
                     });
}

} // namespace VM
} // namespace WasmEdge
