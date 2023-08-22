// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "wasinnenv.h"
#include "wasinnmodule.h"

namespace WasmEdge {
namespace Host {

namespace WASINN {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiNNModule;
}

std::map<std::string_view, Backend> BackendMap = {
    {"OpenVINO"sv, Backend::OpenVINO},
    {"ONNX"sv, Backend::ONNX},
    {"Tensorflow"sv, Backend::Tensorflow},
    {"PyTorch"sv, Backend::PyTorch},
    {"TensorflowLite"sv, Backend::TensorflowLite},
    {"Autodetect"sv, Backend::Autodetect}};

std::map<std::string_view, Device> DeviceMap = {
    {"CPU"sv, Device::CPU}, {"GPU"sv, Device::GPU}, {"TPU"sv, Device::TPU}};

bool load(const std::filesystem::path &Path, std::vector<uint8_t> &Data) {
  std::ifstream File(Path, std::ios::binary);
  if (!File.is_open()) {
    spdlog::error("[WASI-NN] Preload model fail."sv);
    return false;
  }
  File.seekg(0, std::ios::end);
  std::streampos FileSize = File.tellg();
  File.seekg(0, std::ios::beg);
  Data.resize(FileSize);
  File.read(reinterpret_cast<char *>(Data.data()), FileSize);
  File.close();
  return true;
}

WasiNNEnvironment::WasiNNEnvironment() noexcept {
  // Preload NN Models
  for (const auto &M : NNModels.value()) {
    std::istringstream ISS(M);
    const char Delimiter = ':';
    std::string Name;
    std::string Encode;
    std::string Target;
    std::vector<std::string> Paths;
    std::getline(ISS, Name, Delimiter);
    std::getline(ISS, Encode, Delimiter);
    std::getline(ISS, Target, Delimiter);
    std::string Path;
    while (std::getline(ISS, Path, Delimiter)) {
      Paths.push_back(Path);
    }
    std::vector<std::vector<uint8_t>> Models;
    Models.reserve(Paths.size());
    auto Backend = BackendMap.find(Encode);
    auto Device = DeviceMap.find(Target);
    if (Backend != BackendMap.end() && Device != DeviceMap.end()) {
      for (const std::string &Path : Paths) {
        std::vector<uint8_t> Model;
        if (load(std::filesystem::u8path(Path), Model)) {
          Models.push_back(std::move(Model));
        }
      }
      RawMdMap.emplace(Name, std::make_tuple(std::move(Models), Backend->second,
                                             Device->second));
    } else {
      spdlog::error(
          "[WASI-NN] Preload Model's Backend or Device is Not Support."sv);
    }
  }
  NNGraph.reserve(16U);
  NNContext.reserve(16U);
}

PO::List<std::string> WasiNNEnvironment::NNModels(
    PO::Description(
        "Allow preload models from wasinn plugin. Each NN model can be specified as --nn-preload `COMMAND`."sv),
    PO::MetaVar("COMMANDS"sv));

void addOptions(const Plugin::Plugin::PluginDescriptor *,
                PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("nn-preload"sv, WasiNNEnvironment::NNModels);
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasi_nn",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 1, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasi_nn",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = addOptions,
};

} // namespace WASINN

Plugin::PluginRegister WASINN::WasiNNEnvironment::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
