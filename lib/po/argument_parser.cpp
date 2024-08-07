// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "po/argument_parser.h"
#include "common/defines.h"
#include "common/spdlog.h"
#include "system/winapi.h"
#include <cstdio>

namespace WasmEdge {
namespace PO {

cxx20::expected<bool, Error> ArgumentParser::SubCommandDescriptor::parse(
    std::FILE *Out, Span<const char *> ProgramNamePrefix, int Argc,
    const char *Argv[], int ArgP, const bool &VersionOpt) noexcept {
  ProgramNames.reserve(ProgramNamePrefix.size() + 1);
  ProgramNames.assign(ProgramNamePrefix.begin(), ProgramNamePrefix.end());
  if (ArgP < Argc) {
    ProgramNames.push_back(Argv[ArgP]);
  }
  ArgumentDescriptor *CurrentDesc = nullptr;
  bool FirstNonOption = true;
  bool Escaped = false;
  auto PositionalIter = PositionalList.cbegin();
  for (int ArgI = ArgP + 1; ArgI < Argc; ++ArgI) {
    std::string_view Arg = Argv[ArgI];
    if (!Escaped && Arg.size() >= 2 && Arg[0] == '-') {
      if (Arg[1] == '-') {
        if (Arg.size() == 2) {
          Escaped = true;
        } else {
          // long option
          if (CurrentDesc && CurrentDesc->nargs() == 0) {
            CurrentDesc->default_value();
          }
          if (auto Res = consume_long_option_with_argument(Arg); !Res) {
            return cxx20::unexpected(Res.error());
          } else {
            CurrentDesc = *Res;
          }
        }
      } else {
        // short options
        if (CurrentDesc && CurrentDesc->nargs() == 0) {
          CurrentDesc->default_value();
        }
        if (auto Res = consume_short_options(Arg); !Res) {
          return cxx20::unexpected(Res.error());
        } else {
          CurrentDesc = *Res;
        }
      }
    } else if (!Escaped && CurrentDesc) {
      consume_argument(*CurrentDesc, Arg);
      CurrentDesc = nullptr;
    } else {
      // no more options
      if (FirstNonOption) {
        FirstNonOption = false;
        if (!SubCommandMap.empty()) {
          if (auto Iter = SubCommandMap.find(Arg);
              Iter != SubCommandMap.end()) {
            auto &Child = this[Iter->second];
            Child.SC->select();
            return Child.parse(Out, ProgramNames, Argc, Argv, ArgI, VersionOpt);
          }
        }
      }
      Escaped = true;
      if (CurrentDesc) {
        if (auto Res = consume_argument(*CurrentDesc, Arg); !Res) {
          return cxx20::unexpected(Res.error());
        } else {
          CurrentDesc = *Res;
        }
      } else {
        if (PositionalIter == PositionalList.cend()) {
          return cxx20::unexpected<Error>(
              std::in_place, ErrCode::InvalidArgument,
              "positional argument exceeds maximum consuming."s);
        }
        if (auto Res =
                consume_argument(ArgumentDescriptors[*PositionalIter], Arg);
            !Res) {
          return cxx20::unexpected(Res.error());
        } else {
          CurrentDesc = *Res;
        }
        ++PositionalIter;
      }
    }
  }
  if (CurrentDesc && CurrentDesc->nargs() == 0) {
    CurrentDesc->default_value();
  }

  if (VersionOpt) {
    return true;
  }
  if (!HelpOpt->value()) {
    for (const auto &Desc : ArgumentDescriptors) {
      if (Desc.nargs() < Desc.min_nargs()) {
        help(Out);
        return false;
      }
    }
  } else {
    help(Out);
    return true;
  }
  return true;
}

void ArgumentParser::SubCommandDescriptor::usage(
    std::FILE *Out) const noexcept {
  fmt::print(Out, "{}USAGE{}\n"sv, YELLOW_COLOR, RESET_COLOR);
  for (const char *Part : ProgramNames) {
    fmt::print(Out, "\t{}"sv, Part);
  }
  if (!SubCommandList.empty()) {
    fmt::print(Out, " [SUBCOMMANDS]"sv);
  }
  if (!NonpositionalList.empty()) {
    fmt::print(Out, " [OPTIONS]"sv);
  }
  bool First = true;
  for (const auto &Index : PositionalList) {
    const auto &Desc = ArgumentDescriptors[Index];
    if (Desc.hidden()) {
      continue;
    }

    if (First) {
      fmt::print(Out, " [--]"sv);
      First = false;
    }

    const bool Optional = (Desc.min_nargs() == 0);
    fmt::print(Out, " "sv);
    if (Optional) {
      fmt::print(Out, "["sv);
    }
    switch (ArgumentDescriptors[Index].max_nargs()) {
    case 0:
      break;
    case 1:
      fmt::print(Out, "{}"sv, Desc.meta());
      break;
    default:
      fmt::print(Out, "{} ..."sv, Desc.meta());
      break;
    }
    if (Optional) {
      fmt::print(Out, "]"sv);
    }
  }
  fmt::print(Out, "\n"sv);
}

void ArgumentParser::SubCommandDescriptor::help(std::FILE *Out) const noexcept {
// For enabling Windows PowerShell color support.
#if WASMEDGE_OS_WINDOWS && WINAPI_PARTITION_DESKTOP
  winapi::HANDLE_ OutputHandler =
      winapi::GetStdHandle(winapi::STD_OUTPUT_HANDLE_);
  if (OutputHandler != winapi::INVALID_HANDLE_VALUE_) {
    winapi::DWORD_ ConsoleMode = 0;
    if (winapi::GetConsoleMode(OutputHandler, &ConsoleMode)) {
      ConsoleMode |= winapi::ENABLE_VIRTUAL_TERMINAL_PROCESSING_;
      winapi::SetConsoleMode(OutputHandler, ConsoleMode);
    }
  }
#endif

  usage(Out);
  const constexpr std::string_view kIndent = "\t"sv;

  fmt::print(Out, "\n"sv);
  if (!SubCommandList.empty()) {
    fmt::print(Out, "{}SUBCOMMANDS{}\n"sv, YELLOW_COLOR, RESET_COLOR);
    for (const auto Offset : SubCommandList) {
      fmt::print(Out, "{}{}"sv, kIndent, GREEN_COLOR);
      bool First = true;
      for (const auto &Name : this[Offset].SubCommandNames) {
        if (!First) {
          fmt::print(Out, "|"sv);
        }
        fmt::print(Out, "{}"sv, Name);
        First = false;
      }
      fmt::print(Out, "{}\n"sv, RESET_COLOR);
      indent_output(Out, kIndent, 2, 80, this[Offset].SC->description());
      fmt::print(Out, "\n"sv);
    }
    fmt::print(Out, "\n"sv);
  }

  fmt::print(Out, "{}OPTIONS{}\n"sv, YELLOW_COLOR, RESET_COLOR);
  for (const auto &Index : NonpositionalList) {
    const auto &Desc = ArgumentDescriptors[Index];
    if (Desc.hidden()) {
      continue;
    }

    fmt::print(Out, "{}{}\n"sv, kIndent, GREEN_COLOR);
    bool First = true;
    for (const auto &Option : Desc.options()) {
      if (!First) {
        fmt::print(Out, "|"sv);
      }
      if (Option.size() == 1) {
        fmt::print(Out, "-{}"sv, Option);
      } else {
        fmt::print(Out, "--{}"sv, Option);
      }
      First = false;
    }
    fmt::print(Out, "{}\n"sv, RESET_COLOR);
    indent_output(Out, kIndent, 2, 80, Desc.description());
    fmt::print(Out, "\n"sv);
  }
}

void ArgumentParser::SubCommandDescriptor::indent_output(
    std::FILE *Out, const std::string_view kIndent, std::size_t IndentCount,
    std::size_t ScreenWidth, std::string_view Desc) const noexcept {
  const std::size_t Width = ScreenWidth - kIndent.size() * IndentCount;
  while (Desc.size() > Width) {
    const std::size_t SpacePos = Desc.find_last_of(' ', Width);
    if (SpacePos != std::string_view::npos) {
      for (std::size_t I = 0; I < IndentCount; ++I) {
        fmt::print(Out, "{}"sv, kIndent);
      }
      fmt::print(Out, "{}\n"sv, Desc.substr(0, SpacePos));
      const std::size_t WordPos = Desc.find_first_not_of(' ', SpacePos);
      if (WordPos != std::string_view::npos) {
        Desc = Desc.substr(WordPos);
      } else {
        Desc = {};
      }
    }
  }
  if (!Desc.empty()) {
    for (std::size_t I = 0; I < IndentCount; ++I) {
      fmt::print(Out, "{}"sv, kIndent);
    }
    fmt::print(Out, "{}"sv, Desc);
  }
}

cxx20::expected<ArgumentParser::ArgumentDescriptor *, Error>
ArgumentParser::SubCommandDescriptor::consume_short_options(
    std::string_view Arg) noexcept {
  ArgumentDescriptor *CurrentDesc = nullptr;
  for (std::size_t I = 1; I < Arg.size(); ++I) {
    if (CurrentDesc && CurrentDesc->nargs() == 0) {
      CurrentDesc->default_value();
    }
    std::string_view Option = Arg.substr(I, 1);
    if (auto Res = consume_short_option(Option); !Res) {
      return cxx20::unexpected(Res.error());
    } else {
      CurrentDesc = *Res;
    }
  }
  return CurrentDesc;
}

cxx20::expected<ArgumentParser::ArgumentDescriptor *, Error>
ArgumentParser::SubCommandDescriptor::consume_long_option_with_argument(
    std::string_view Arg) noexcept {
  if (auto Pos = Arg.find('=', 2); Pos != std::string_view::npos) {
    // long option with argument
    std::string_view Option = Arg.substr(2, Pos - 2);
    std::string_view Argument = Arg.substr(Pos + 1);
    if (auto Res = consume_long_option(Option); !Res) {
      return cxx20::unexpected<Error>(Res.error());
    } else if (ArgumentDescriptor *CurrentDesc = *Res; !CurrentDesc) {
      return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                      "option "s + std::string(Option) +
                                          "doesn't need arguments."s);
    } else {
      consume_argument(*CurrentDesc, Argument);
      return nullptr;
    }
  } else {
    // long option without argument
    std::string_view Option = Arg.substr(2);
    return consume_long_option(Option);
  }
}

cxx20::expected<ArgumentParser::ArgumentDescriptor *, Error>
ArgumentParser::SubCommandDescriptor::consume_short_option(
    std::string_view Option) noexcept {
  auto Iter = ArgumentMap.find(Option);
  if (Iter == ArgumentMap.end()) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                    "unknown option: "s + std::string(Option));
  }
  ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
  if (CurrentDesc.max_nargs() == 0) {
    CurrentDesc.default_value();
    return nullptr;
  }
  return &CurrentDesc;
}

cxx20::expected<ArgumentParser::ArgumentDescriptor *, Error>
ArgumentParser::SubCommandDescriptor::consume_long_option(
    std::string_view Option) noexcept {
  auto Iter = ArgumentMap.find(Option);
  if (Iter == ArgumentMap.end()) {
    return cxx20::unexpected<Error>(std::in_place, ErrCode::InvalidArgument,
                                    "unknown option: "s + std::string(Option));
  }
  ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
  if (CurrentDesc.max_nargs() == 0) {
    CurrentDesc.default_value();
    return nullptr;
  }
  return &CurrentDesc;
}

cxx20::expected<ArgumentParser::ArgumentDescriptor *, Error>
ArgumentParser::SubCommandDescriptor::consume_argument(
    ArgumentDescriptor &CurrentDesc, std::string_view Argument) noexcept {
  if (auto Res = CurrentDesc.argument(std::string(Argument)); !Res) {
    return cxx20::unexpected(Res.error());
  }
  if (++CurrentDesc.nargs() >= CurrentDesc.max_nargs()) {
    return nullptr;
  }
  return &CurrentDesc;
}

bool ArgumentParser::parse(std::FILE *Out, int Argc,
                           const char *Argv[]) noexcept {
  if (auto Res = SubCommandDescriptors.front().parse(Out, {}, Argc, Argv, 0,
                                                     VerOpt.value());
      !Res) {
    fmt::print(Out, "{}\n"sv, Res.error().message());
    return false;
  } else {
    return *Res || VerOpt.value();
  }
}

} // namespace PO
} // namespace WasmEdge
