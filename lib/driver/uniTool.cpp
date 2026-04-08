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

  auto Parser = PO::ArgumentParser();

  PO::SubCommand ToolSubCommand(
      PO::Description("Wasmedge runtime tool subcommand"sv));
  PO::SubCommand CompilerSubCommand(
      PO::Description("Wasmedge compiler subcommand"sv));
  PO::SubCommand ParseSubCommand(
      PO::Description("Wasmedge parse tool subcommand"sv));
  PO::SubCommand InstantiateSubCommand(
      PO::Description("Wasmedge instantiate tool subcommand"sv));
  PO::SubCommand ValidateSubCommand(
      PO::Description("Wasmedge validate tool subcommand"sv));
  struct DriverToolOptions ToolOptions;
  struct DriverCompilerOptions CompilerOptions;
  struct DriverToolOptions ParseOptions;
  struct DriverToolOptions InstantiateOptions;
  struct DriverToolOptions ValidateOptions;

  // Construct Parser Subcommands and Options
  if (ToolSelect == ToolType::All) {
    ToolOptions.addOptions(Parser);

    Parser.begin_subcommand(CompilerSubCommand, "compile"sv);
    CompilerOptions.addOptions(Parser);
    Parser.end_subcommand();
    Parser.begin_subcommand(ToolSubCommand, "run"sv);
    ToolOptions.addOptions(Parser);
    Parser.end_subcommand();
    Parser.begin_subcommand(ParseSubCommand, "parse"sv);
    ParseOptions.addParserOptions(Parser);
    Parser.end_subcommand();
    Parser.begin_subcommand(InstantiateSubCommand, "instantiate"sv);
    InstantiateOptions.addLinkerOptions(Parser);
    Parser.end_subcommand();
    Parser.begin_subcommand(ValidateSubCommand, "validate"sv);
    ValidateOptions.addParserOptions(Parser);
    Parser.end_subcommand();
  } else if (ToolSelect == ToolType::Tool) {
    ToolOptions.addOptions(Parser);
  } else if (ToolSelect == ToolType::Compiler) {
    CompilerOptions.addOptions(Parser);
  } else if (ToolSelect == ToolType::Parse) {
    ParseOptions.addParserOptions(Parser);
  } else if (ToolSelect == ToolType::Validate) {
    ValidateOptions.addParserOptions(Parser);
  } else if (ToolSelect == ToolType::Instantiate) {
    InstantiateOptions.addLinkerOptions(Parser);
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

  if (!ParseSubCommand.is_selected() && !ValidateSubCommand.is_selected() &&
      !InstantiateSubCommand.is_selected()) {
    const std::string &Level = ToolOptions.LogLevel.value();
    if (!Log::setLoggingLevelFromString(Level)) {
      spdlog::warn(
          "Invalid log level: {}. Valid values are: off, trace, debug, "
          "info, warning, error, fatal. Falling back to info level."sv,
          Level);
      Log::setInfoLoggingLevel();
    }
  }

  // Forward Results
  if (ToolSubCommand.is_selected() || ToolSelect == ToolType::Tool) {
    return Tool(ToolOptions);
  } else if (CompilerSubCommand.is_selected() ||
             ToolSelect == ToolType::Compiler) {
    return Compiler(CompilerOptions);
  } else if (ParseSubCommand.is_selected() || ToolSelect == ToolType::Parse) {
    return ParseTool(ParseOptions);
  } else if (ValidateSubCommand.is_selected() ||
             ToolSelect == ToolType::Validate) {
    return ValidateTool(ValidateOptions);
  } else if (InstantiateSubCommand.is_selected() ||
             ToolSelect == ToolType::Instantiate) {
    return InstantiateTool(InstantiateOptions);
  } else {
    return Tool(ToolOptions);
  }
}
} // namespace Driver
} // namespace WasmEdge
