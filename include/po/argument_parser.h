// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/po/argument_parser.h - Argument parser -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"
#include "po/error.h"
#include "po/list.h"
#include "po/option.h"
#include "po/subcommand.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace PO {

using namespace std::literals;
class ArgumentParser {
private:
  class ArgumentDescriptor {
  public:
    template <typename T>
    ArgumentDescriptor(T &Opt) noexcept
        : Desc(Opt.description()), Meta(Opt.meta()), MinNArgs(Opt.min_narg()),
          MaxNArgs(Opt.max_narg()), Argument([&Opt](std::string Arg) {
            return Opt.argument(std::move(Arg));
          }),
          DefaultValue([&Opt]() { Opt.default_argument(); }),
          Hidden(Opt.hidden()), Store(&Opt.value()) {}
    auto &nargs() noexcept { return NArgs; }
    auto &nargs() const noexcept { return NArgs; }
    auto &options() noexcept { return Options; }
    auto &options() const noexcept { return Options; }

    auto &description() const noexcept { return Desc; }
    auto &meta() const noexcept { return Meta; }
    auto &hidden() const noexcept { return Hidden; }
    auto &min_nargs() const noexcept { return MinNArgs; }
    auto &max_nargs() const noexcept { return MaxNArgs; }
    cxx20::expected<void, Error> argument(std::string Arg) const noexcept {
      return Argument(std::move(Arg));
    }
    void default_value() const noexcept { DefaultValue(); }
    template <typename T> void raw_value(T Value) const noexcept {
      *static_cast<T *>(Store) = Value;
    }

  private:
    std::string_view Desc;
    std::string_view Meta;
    std::size_t NArgs = 0;
    std::size_t MinNArgs;
    std::size_t MaxNArgs;
    std::vector<std::string_view> Options;
    std::function<cxx20::expected<void, Error>(std::string)> Argument;
    std::function<void()> DefaultValue;
    bool Hidden;
    void *Store;
  };

  class SubCommandDescriptor {
  public:
    SubCommandDescriptor() noexcept
        : HelpOpt(std::make_unique<Option<Toggle>>(
              Description("Show this help messages"sv))) {
      add_option("h"sv, *HelpOpt);
      add_option("help"sv, *HelpOpt);
    }
    SubCommandDescriptor(SubCommand &SC) noexcept : SubCommandDescriptor() {
      this->SC = &SC;
    }

    template <typename... ArgsT>
    void add_child(SubCommandDescriptor &Child, ArgsT &&...Args) noexcept {
      const size_t Offset = static_cast<size_t>(&Child - this);
      SubCommandList.push_back(Offset);
      (Child.SubCommandNames.push_back(Args), ...);
      (SubCommandMap.emplace(std::forward<ArgsT>(Args), Offset), ...);
    }

    template <typename T>
    void add_option(std::string_view Argument, T &Opt) noexcept {
      if (auto Iter = OptionMap.find(std::addressof(Opt));
          Iter == OptionMap.end()) {
        OptionMap.emplace(std::addressof(Opt), ArgumentDescriptors.size());
        ArgumentMap.emplace(Argument, ArgumentDescriptors.size());
        NonpositionalList.push_back(ArgumentDescriptors.size());
        ArgumentDescriptors.emplace_back(Opt);
        ArgumentDescriptors.back().options().push_back(Argument);
      } else {
        ArgumentMap.emplace(Argument, Iter->second);
        ArgumentDescriptors[Iter->second].options().push_back(Argument);
      }
    }

    template <typename T> void add_option(T &Opt) noexcept {
      if (auto Iter = OptionMap.find(std::addressof(Opt));
          Iter == OptionMap.end()) {
        OptionMap.emplace(std::addressof(Opt), ArgumentDescriptors.size());
        PositionalList.emplace_back(ArgumentDescriptors.size());
        ArgumentDescriptors.emplace_back(Opt);
      } else {
        PositionalList.emplace_back(Iter->second);
      }
    }

    bool set_raw_value(std::string_view Option) const noexcept {
      auto Iter = ArgumentMap.find(Option);
      if (Iter == ArgumentMap.end()) {
        return false;
      }
      const ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
      if (CurrentDesc.max_nargs() != 0) {
        return false;
      }

      CurrentDesc.default_value();
      return true;
    }

    template <typename T>
    bool set_raw_value(std::string_view Option, T Value) const noexcept {
      auto Iter = ArgumentMap.find(Option);
      if (Iter == ArgumentMap.end()) {
        return false;
      }
      const ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
      if (CurrentDesc.max_nargs() == 0) {
        return false;
      }

      CurrentDesc.raw_value(Value);
      return true;
    }

    cxx20::expected<bool, Error> parse(std::FILE *Out,
                                       Span<const char *> ProgramNamePrefix,
                                       int Argc, const char *Argv[], int ArgP,
                                       const bool &VersionOpt) noexcept;

    void usage(std::FILE *Out) const noexcept;

    void help(std::FILE *Out) const noexcept;

    void indent_output(std::FILE *Out, const std::string_view kIndent,
                       std::size_t IndentCount, std::size_t ScreenWidth,
                       std::string_view Desc) const noexcept;
    bool isHelp() const noexcept { return HelpOpt->value(); }

  private:
    cxx20::expected<ArgumentDescriptor *, Error>
    consume_short_options(std::string_view Arg) noexcept;

    cxx20::expected<ArgumentDescriptor *, Error>
    consume_long_option_with_argument(std::string_view Arg) noexcept;

    cxx20::expected<ArgumentDescriptor *, Error>
    consume_short_option(std::string_view Option) noexcept;

    cxx20::expected<ArgumentDescriptor *, Error>
    consume_long_option(std::string_view Option) noexcept;

    cxx20::expected<ArgumentDescriptor *, Error>
    consume_argument(ArgumentDescriptor &CurrentDesc,
                     std::string_view Argument) noexcept;

    SubCommand *SC = nullptr;
    std::vector<std::string_view> SubCommandNames;
    std::vector<const char *> ProgramNames;
    std::vector<ArgumentDescriptor> ArgumentDescriptors;
    std::unordered_map<void *, std::size_t> OptionMap;
    std::unordered_map<std::string_view, std::size_t> ArgumentMap;
    std::unordered_map<std::string_view, std::size_t> SubCommandMap;
    std::vector<std::size_t> SubCommandList;
    std::vector<std::size_t> NonpositionalList;
    std::vector<std::size_t> PositionalList;
    std::unique_ptr<Option<Toggle>> HelpOpt;

#if WASMEDGE_OS_WINDOWS
    // Native PowerShell failed to display yellow, so we use bright yellow.
    static inline constexpr std::string_view YELLOW_COLOR = "\x1b[93m"sv;
#else
    static inline constexpr std::string_view YELLOW_COLOR = "\x1b[33m"sv;
#endif
    static inline constexpr std::string_view GREEN_COLOR = "\x1b[32m";
    static inline constexpr std::string_view RESET_COLOR = "\x1b[0m";
  };

public:
  ArgumentParser() noexcept
      : SubCommandDescriptors(1), CurrentSubCommandId(0),
        VerOpt(Description("Show version information"sv)) {
    SubCommandDescriptors.front().add_option("v"sv, VerOpt);
    SubCommandDescriptors.front().add_option("version"sv, VerOpt);
  }

  template <typename T>
  ArgumentParser &add_option(std::string_view Argument, T &Opt) noexcept {
    SubCommandDescriptors[CurrentSubCommandId].add_option(Argument, Opt);
    return *this;
  }

  template <typename T> ArgumentParser &add_option(T &Opt) noexcept {
    SubCommandDescriptors[CurrentSubCommandId].add_option(Opt);
    return *this;
  }

  bool set_raw_value(std::string_view Option) const noexcept {
    return SubCommandDescriptors[CurrentSubCommandId].set_raw_value(Option);
  }

  template <typename T>
  bool set_raw_value(std::string_view Option, T Value) const noexcept {
    return SubCommandDescriptors[CurrentSubCommandId].set_raw_value(Option,
                                                                    Value);
  }

  template <typename... ArgsT>
  ArgumentParser &begin_subcommand(SubCommand &SC, ArgsT &&...Args) noexcept {
    SubCommandStack.push_back(CurrentSubCommandId);
    const auto ParentSubCommandId =
        std::exchange(CurrentSubCommandId, SubCommandDescriptors.size());
    SubCommandDescriptors.emplace_back(SC);
    SubCommandDescriptors[ParentSubCommandId].add_child(
        SubCommandDescriptors[CurrentSubCommandId],
        std::forward<ArgsT>(Args)...);
    return *this;
  }

  ArgumentParser &end_subcommand() noexcept {
    CurrentSubCommandId = SubCommandStack.back();
    SubCommandStack.pop_back();
    return *this;
  }

  bool parse(std::FILE *Out, int Argc, const char *Argv[]) noexcept;

  void usage(std::FILE *Out) const noexcept {
    SubCommandDescriptors.front().usage(Out);
  }
  void help(std::FILE *Out) const noexcept {
    SubCommandDescriptors.front().help(Out);
  }
  bool isVersion() const noexcept { return VerOpt.value(); }
  bool isHelp() const noexcept {
    bool is_help_select = false;
    for (const auto &iter : SubCommandDescriptors) {
      is_help_select |= iter.isHelp();
    }
    return is_help_select;
  }

private:
  std::vector<SubCommandDescriptor> SubCommandDescriptors;
  std::size_t CurrentSubCommandId;
  std::vector<std::size_t> SubCommandStack;
  Option<Toggle> VerOpt;
};

} // namespace PO
} // namespace WasmEdge
