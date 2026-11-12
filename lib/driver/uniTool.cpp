// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "driver/unitool.h"
#include "common/spdlog.h"
#include "common/version.h"
#include "driver/compiler.h"
#include "driver/tool.h"
#include "plugin/plugin.h"
#include "po/argument_parser.h"

#include <cstdlib>
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

  auto AddOptions = [&](const ToolType Type) noexcept {
    switch (Type) {
    case ToolType::Tool:
      ToolOptions.addOptions(Parser);
      return true;
    case ToolType::Compiler:
      CompilerOptions.addOptions(Parser);
      return true;
    case ToolType::Parse:
      ParseOptions.addParserOptions(Parser);
      return true;
    case ToolType::Instantiate:
      InstantiateOptions.addLinkerOptions(Parser);
      return true;
    case ToolType::Validate:
      ValidateOptions.addParserOptions(Parser);
      return true;
    default:
      return false;
    }
  };
  auto AddSubCommand = [&](PO::SubCommand &SubCommand, std::string_view Name,
                           const ToolType Type) noexcept {
    Parser.begin_subcommand(SubCommand, Name);
    AddOptions(Type);
    Parser.end_subcommand();
  };

  // Construct Parser Subcommands and Options
  if (ToolSelect == ToolType::All) {
    AddOptions(ToolType::Tool);
    AddSubCommand(CompilerSubCommand, "compile"sv, ToolType::Compiler);
    AddSubCommand(ToolSubCommand, "run"sv, ToolType::Tool);
    AddSubCommand(ParseSubCommand, "parse"sv, ToolType::Parse);
    AddSubCommand(InstantiateSubCommand, "instantiate"sv,
                  ToolType::Instantiate);
    AddSubCommand(ValidateSubCommand, "validate"sv, ToolType::Validate);
  } else if (!AddOptions(ToolSelect)) {
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
  ToolType Selected = ToolSelect;
  if (ToolSelect == ToolType::All) {
    if (CompilerSubCommand.is_selected()) {
      Selected = ToolType::Compiler;
    } else if (ParseSubCommand.is_selected()) {
      Selected = ToolType::Parse;
    } else if (InstantiateSubCommand.is_selected()) {
      Selected = ToolType::Instantiate;
    } else if (ValidateSubCommand.is_selected()) {
      Selected = ToolType::Validate;
    } else {
      Selected = ToolType::Tool;
    }
  }
  switch (Selected) {
  case ToolType::Compiler:
    CompilerOptions.inheritLoggingOptions(ToolOptions);
    return Compiler(CompilerOptions);
  case ToolType::Parse:
    ParseOptions.inheritGlobalOptions(ToolOptions);
    return ParseTool(ParseOptions);
  case ToolType::Instantiate:
    InstantiateOptions.inheritGlobalOptions(ToolOptions);
    return InstantiateTool(InstantiateOptions);
  case ToolType::Validate:
    ValidateOptions.inheritGlobalOptions(ToolOptions);
    return ValidateTool(ValidateOptions);
  default:
    return Tool(ToolOptions);
  }
}

} // namespace Driver
} // namespace WasmEdge
