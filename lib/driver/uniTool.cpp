// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "driver/unitool.h"
#include "common/spdlog.h"
#include "driver/compiler.h"
#include "driver/tool.h"
#include "po/argument_parser.h"

#include <string_view>

namespace WasmEdge {
namespace Driver {

int UniTool(int Argc, const char *Argv[], const ToolType ToolSelect) noexcept {
  using namespace std::literals;

  std::ios::sync_with_stdio(false);
  Log::setInfoLoggingLevel();

  auto Parser = PO::ArgumentParser();

  PO::SubCommand ToolSubCommand(
      PO::Description("Wasmedge runtime tool subcommand"sv));
  PO::SubCommand CompilerSubCommand(
      PO::Description("Wasmedge compiler subcommand"sv));
  struct DriverToolOptions ToolOptions;
  struct DriverCompilerOptions CompilerOptions;

  // Construct Parser Subcommands and Options
  if (ToolSelect == ToolType::All) {
    ToolOptions.add_option(Parser);

    Parser.begin_subcommand(CompilerSubCommand, "compile"sv);
    CompilerOptions.add_option(Parser);
    Parser.end_subcommand();

    Parser.begin_subcommand(ToolSubCommand, "run"sv);
    ToolOptions.add_option(Parser);
    Parser.end_subcommand();
  } else if (ToolSelect == ToolType::Tool) {
    ToolOptions.add_option(Parser);
  } else if (ToolSelect == ToolType::Compiler) {
    CompilerOptions.add_option(Parser);
  } else {
    return EXIT_FAILURE;
  }

  // Parse
  if (!Parser.parse(stdout, Argc, Argv)) {
    return EXIT_FAILURE;
  }
  if (Parser.isVersion()) {
    fmt::print("{} version {}\n"sv, Argv[0], kVersionString);
    for (const auto &Plugin : Plugin::Plugin::plugins()) {
      auto PluginVersion = Plugin.version();
      fmt::print("{} (plugin \"{}\") version {}.{}.{}.{}\n"sv,
                 Plugin.path().string(), Plugin.name(), PluginVersion.Major,
                 PluginVersion.Minor, PluginVersion.Patch, PluginVersion.Build);
    }
    return EXIT_SUCCESS;
  }
  if (Parser.isHelp()) {
    return EXIT_SUCCESS;
  }

  // Forward Results
  if (ToolSubCommand.is_selected() || ToolSelect == ToolType::Tool) {
    return Tool(ToolOptions);
  } else if (CompilerSubCommand.is_selected() ||
             ToolSelect == ToolType::Compiler) {
    return Compiler(CompilerOptions);
  } else {
    return Tool(ToolOptions);
  }
}
} // namespace Driver
} // namespace WasmEdge
