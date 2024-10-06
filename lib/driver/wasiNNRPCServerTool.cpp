#include "common/spdlog.h"
#include "driver/wasi_nn_rpc/wasi_nn_rpcserver/wasi_nn_rpcserver.h"
#include "plugin/plugin.h"
#include "po/argument_parser.h"

#include <grpcpp/server_builder.h>
#include <string_view>

using namespace std::literals;
using namespace WasmEdge;

namespace WasmEdge {
namespace Driver {

void loadPlugins(void) {
  Plugin::Plugin::loadFromDefaultPaths();
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    spdlog::info("Loaded Plugin: {} from path: {}", Plugin.name(),
                 Plugin.path());
  }
}

Runtime::Instance::ModuleInstance *createWasiNNModule() {
  if (const auto *Plugin = Plugin::Plugin::find("wasi_nn"sv)) {
    if (const auto *Module = Plugin->findModule("wasi_nn"sv)) {
      return Module->create().release();
    }
  }
  return nullptr;
}

int WasiNNRPCServer(int Argc, const char *Argv[]) noexcept {
  std::ios::sync_with_stdio(false);
  Log::setInfoLoggingLevel();
  setenv("_WASI_NN_RPCSERVER", "1", 1); // wasi_nn plugin checks this env var

  // Parse the args
  PO::Option<std::string> NNRPCURI(
      PO::Description("Specify NN RPC URI to serve (\"unix://...\")"sv),
      PO::MetaVar("URI"sv), PO::DefaultValue(std::string("")));
  auto Parser = PO::ArgumentParser();
  Parser.add_option("nn-rpc-uri"sv, NNRPCURI);
  loadPlugins();
  Plugin::Plugin::addPluginOptions(Parser); // Register "nn-preload", etc.
  if (!Parser.parse(stdout, Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isHelp()) {
    return EXIT_SUCCESS;
  }
  auto URI = NNRPCURI.value();
  if (URI.empty()) {
    spdlog::error("--nn-rpc-uri has to be specified"sv);
    return EXIT_FAILURE;
  }

  // Create the wasi_nn module
  auto *NNMod = createWasiNNModule();
  if (NNMod == nullptr) {
    spdlog::error(
        "Failed to get the wasi_nn module (Hint: set $WASMEDGE_PLUGIN_PATH to "
        "the directory where libwasmedgePluginWasiNN.* exists"sv);
    return EXIT_FAILURE;
  }

  // Create the services
  WasiNNRPC::Server::ServiceSet ServiceSet(*NNMod);

  // Create the gRPC server
  grpc::ServerBuilder Builder;
  spdlog::info("Listening on \"{}\""sv, URI);
  std::string_view UnixPrefix = "unix://";
  if (URI.substr(0, UnixPrefix.length()) != UnixPrefix) {
    spdlog::warn("Expected \"unix://...\", got \"{}\""sv, URI);
  }
  auto Cred = grpc::InsecureServerCredentials(); // safe for unix://...
  Builder.AddListeningPort(URI, Cred);
  for (auto *Service : ServiceSet.services()) {
    Builder.RegisterService(Service);
  }

  // Start the gRPC server
  auto Server = Builder.BuildAndStart();
  if (Server == nullptr) {
    return EXIT_FAILURE;
  }
  Server->Wait();
  return EXIT_SUCCESS;
}
} // namespace Driver
} // namespace WasmEdge
