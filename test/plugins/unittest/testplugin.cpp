// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

PO::Option<PO::Toggle>
    WasmEdgePluginTestEnv::CmdOpt(PO::Description("Test for option."sv));

namespace {

void addOptions(const Plugin::Plugin::PluginDescriptor *,
                PO::ArgumentParser &Parser) noexcept {
  Parser.add_option("arg"sv, WasmEdgePluginTestEnv::CmdArgs)
      .add_option("name"sv, WasmEdgePluginTestEnv::CmdName)
      .add_option("opt"sv, WasmEdgePluginTestEnv::CmdOpt);
}

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgePluginTestModule;
}

static Plugin::PluginModule::ModuleDescriptor MD[]{
    {
        /* Name */ "wasmedge_plugintest_cpp_module",
        /* Description */ "This is for the plugin tests in WasmEdge.",
        /* Create */ create,
    },
};

Plugin::Plugin::PluginDescriptor Descriptor{
    /* Name */ "wasmedge_plugintest_cpp",
    /* Description */ "",
    /* APIVersion */ Plugin::Plugin::CurrentAPIVersion,
    /* Version */ {0, 10, 0, 0},
    /* ModuleCount */ 1,
    /* ModuleDescriptions */ MD,
    /* ComponentCount */ 0,
    /* ComponentDescriptions */ nullptr,
    /* AddOptions */ addOptions,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace
} // namespace Host
} // namespace WasmEdge
