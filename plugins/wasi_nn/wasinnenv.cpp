// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinnenv.h"
#include "types.h"
#include "wasinnmodule.h"

#include <sstream>

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
#include <grpc/grpc.h>
#endif

namespace WasmEdge {
namespace Host {

namespace WASINN {

namespace {
Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasiNNModule;
}

std::map<std::string_view, Backend> BackendMap = {
    {"openvino"sv, Backend::OpenVINO},
    {"onnx"sv, Backend::ONNX},
    {"tensorflow"sv, Backend::Tensorflow},
    {"pytorch"sv, Backend::PyTorch},
    {"tensorflowlite"sv, Backend::TensorflowLite},
    {"autodetect"sv, Backend::Autodetect},
    {"ggml"sv, Backend::GGML},
    {"neuralspeed"sv, Backend::NeuralSpeed},
    {"whisper"sv, Backend::Whisper},
    {"piper"sv, Backend::Piper},
    {"chattts"sv, Backend::ChatTTS}};

std::map<std::string_view, Device> DeviceMap = {{"cpu"sv, Device::CPU},
                                                {"gpu"sv, Device::GPU},
                                                {"tpu"sv, Device::TPU},
                                                {"auto"sv, Device::AUTO}};

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
} // namespace

WasiNNEnvironment::WasiNNEnvironment() noexcept {
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (getenv("_WASI_NN_RPCSERVER") == nullptr) {
    // RPC client mode
    auto URI = NNRPCURI.value();
    if (!URI.empty()) {
      std::string_view UnixPrefix = "unix://";
      if (URI.substr(0, UnixPrefix.length()) != UnixPrefix) {
        spdlog::warn("[WASI-NN] Expected \"unix://...\", got \"{}\""sv, URI);
      }
      auto Cred = grpc::InsecureChannelCredentials(); // safe for unix://...
      NNRPCChannel = grpc::CreateChannel(URI, Cred);
      if (NNModels.value().size() > 0) {
        spdlog::warn(
            "[WASI-NN] nn-preload has to be specified on the RPC server side, not on the client side"sv);
      }
      return;
    }
  }
#endif
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
    std::transform(Encode.begin(), Encode.end(), Encode.begin(),
                   [](unsigned char C) {
                     return static_cast<unsigned char>(std::tolower(C));
                   });
    std::transform(Target.begin(), Target.end(), Target.begin(),
                   [](unsigned char C) {
                     return static_cast<unsigned char>(std::tolower(C));
                   });
    auto Backend = BackendMap.find(Encode);
    auto Device = DeviceMap.find(Target);
    if (Backend != BackendMap.end() && Device != DeviceMap.end()) {
      if (Backend->second == Backend::GGML) {
        // In GGML, we only support loading one model from nn-preload config.
        // To handle paths on Windows that contains `:` in the path, we combine
        // the Paths into a single string separated by `:`.
        std::string P;
        for (const std::string &PathSegment : Paths) {
          P += PathSegment;
          if (PathSegment != Paths.back()) {
            P += ":";
          }
        }
        // We write model path to model data to avoid file IO in llama.cpp.
        std::string ModelPath = "preload:" + P;
        std::vector<uint8_t> ModelPathData(ModelPath.begin(), ModelPath.end());
        Models.push_back(std::move(ModelPathData));
      } else {
        for (const std::string &P : Paths) {
          std::vector<uint8_t> Model;
          if (load(std::filesystem::u8path(P), Model)) {
            Models.push_back(std::move(Model));
          }
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

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
PO::Option<std::string> WasiNNEnvironment::NNRPCURI(
    PO::Description("Specify NN RPC URI to connect (\"unix://...\")"sv),
    PO::MetaVar("URI"sv), PO::DefaultValue(std::string("")));
#endif

namespace {
void addOptions(const Plugin::Plugin::PluginDescriptor *,
                PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("nn-preload"sv, WasiNNEnvironment::NNModels);
#ifdef WASMEDGE_BUILD_WASI_NN_RPC
  if (getenv("_WASI_NN_RPCSERVER") == nullptr) {
    // RPC client mode
    Parser.add_option("nn-rpc-uri"sv, WasiNNEnvironment::NNRPCURI);
  }
#endif
}

static Plugin::PluginModule::ModuleDescriptor MD[] = {
    {
        /* Name */ "wasi_nn",
        /* Description */ "",
        /* Create */ create,
    },
};

Plugin::Plugin::PluginDescriptor Descriptor{
    /* Name */ "wasi_nn",
    /* Description */ "",
    /* APIVersion */ Plugin::Plugin::CurrentAPIVersion,
    /* Version */ {0, 10, 1, 0},
    /* ModuleCount */ 1,
    /* ModuleDescriptions */ MD,
    /* ComponentCount */ 0,
    /* ComponentDescriptions */ nullptr,
    /* AddOptions */ addOptions,
};
} // namespace

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace WASINN

} // namespace Host
} // namespace WasmEdge
