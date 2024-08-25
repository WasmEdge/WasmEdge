// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "processenv.h"
#include "processmodule.h"

#include "po/helper.h"

#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals::string_view_literals;

PO::List<std::string> WasmEdgeProcessEnvironment::AllowCmd(
    PO::Description(
        "Allow commands called from wasmedge_process host functions. Each command can be specified as --allow-command `COMMAND`."sv),
    PO::MetaVar("COMMANDS"sv));

PO::Option<PO::Toggle> WasmEdgeProcessEnvironment::AllowCmdAll(PO::Description(
    "Allow all commands called from wasmedge_process host functions."sv));

WasmEdgeProcessEnvironment::WasmEdgeProcessEnvironment() noexcept
    : AllowedCmd(AllowCmd.value().begin(), AllowCmd.value().end()),
      AllowedAll(AllowCmdAll.value()) {}

namespace {

void addOptions(const Plugin::Plugin::PluginDescriptor *,
                PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("allow-command"sv, WasmEdgeProcessEnvironment::AllowCmd)
      .add_option("allow-command-all"sv,
                  WasmEdgeProcessEnvironment::AllowCmdAll);
}

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeProcessModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_process",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_process",
                .Description = "",
                .Create = create,
            },
        },
    .AddOptions = addOptions,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
