// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "testplugin.h"
#include "po/helper.h"

#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals::string_view_literals;

PO::List<std::string>
    WasmEdgePluginTestEnv::CmdArgs(PO::Description("Test for args."sv),
                                   PO::MetaVar("ARG"sv));

PO::Option<std::string>
    WasmEdgePluginTestEnv::CmdName(PO::Description("Test for input name."sv),
                                   PO::DefaultValue(std::string("")));

namespace {

void addOptions(const Plugin::Plugin::PluginDescriptor *,
                PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("arg"sv, WasmEdgePluginTestEnv::CmdArgs)
      .add_option("name"sv, WasmEdgePluginTestEnv::CmdName);
}

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgePluginTestModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_plugintest",
    .Description = "",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 10, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_plugintest",
                .Description = "This is for the plugin tests in WasmEdge.",
                .Create = create,
            },
        },
    .AddOptions = addOptions,
};

} // namespace

Plugin::PluginRegister WasmEdgePluginTestEnv::Register(&Descriptor);

} // namespace Host
} // namespace WasmEdge
