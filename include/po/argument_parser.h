// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/po/argument_parser.h - Argument parser -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "po/list.h"
#include "po/option.h"
#include "po/subcommand.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
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
    ArgumentDescriptor(T &Opt)
        : Desc(Opt.description()), Meta(Opt.meta()), MinNArgs(Opt.min_narg()),
          MaxNArgs(Opt.max_narg()), Value([&Opt](std::string Argument) {
            Opt.argument(std::move(Argument));
          }),
          DefaultValue([&Opt]() { Opt.default_argument(); }),
          Hidden(Opt.hidden()) {}
    auto &nargs() noexcept { return NArgs; }
    auto &nargs() const noexcept { return NArgs; }
    auto &options() noexcept { return Options; }
    auto &options() const noexcept { return Options; }

    auto &description() const noexcept { return Desc; }
    auto &meta() const noexcept { return Meta; }
    auto &hidden() const noexcept { return Hidden; }
    auto &min_nargs() const noexcept { return MinNArgs; }
    auto &max_nargs() const noexcept { return MaxNArgs; }
    void value(std::string String) const noexcept { Value(std::move(String)); }
    void default_value() const noexcept { DefaultValue(); }

  private:
    std::string_view Desc;
    std::string_view Meta;
    std::size_t NArgs = 0;
    std::size_t MinNArgs;
    std::size_t MaxNArgs;
    std::vector<std::string_view> Options;
    std::function<void(std::string)> Value;
    std::function<void()> DefaultValue;
    bool Hidden;
  };

  class SubCommandDescriptor {
  public:
    SubCommandDescriptor()
        : HelpOpt(std::make_unique<Option<Toggle>>(
              Description("Show this help messages"sv))) {
      add_option("h"sv, *HelpOpt);
      add_option("help"sv, *HelpOpt);
    }
    SubCommandDescriptor(SubCommand &SC) : SubCommandDescriptor() {
      this->SC = &SC;
    }

    template <typename... ArgsT>
    void add_child(SubCommandDescriptor &Child, ArgsT &&...Args) {
      const size_t Offset = static_cast<size_t>(&Child - this);
      SubCommandList.push_back(Offset);
      (Child.SubCommandNames.push_back(Args), ...);
      (SubCommandMap.emplace(std::forward<ArgsT>(Args), Offset), ...);
    }

    template <typename T> void add_option(std::string_view Argument, T &Opt) {
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

    template <typename T> void add_option(T &Opt) {
      if (auto Iter = OptionMap.find(std::addressof(Opt));
          Iter == OptionMap.end()) {
        OptionMap.emplace(std::addressof(Opt), ArgumentDescriptors.size());
        PositionalList.emplace_back(ArgumentDescriptors.size());
        ArgumentDescriptors.emplace_back(Opt);
      } else {
        PositionalList.emplace_back(Iter->second);
      }
    }

    bool parse(Span<const char *> ProgramNamePrefix, int Argc,
               const char *Argv[], int ArgP, const bool &VersionOpt) noexcept {
      try {
        ProgramNames.reserve(ProgramNamePrefix.size() + 1);
        ProgramNames.assign(ProgramNamePrefix.begin(), ProgramNamePrefix.end());
        ProgramNames.push_back(Argv[ArgP]);
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
                CurrentDesc = consume_long_option_with_argument(Arg);
              }
            } else {
              // short options
              if (CurrentDesc && CurrentDesc->nargs() == 0) {
                CurrentDesc->default_value();
              }
              CurrentDesc = consume_short_options(Arg);
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
                  return Child.parse(ProgramNames, Argc, Argv, ArgI,
                                     VersionOpt);
                }
              }
            }
            Escaped = true;
            if (CurrentDesc) {
              CurrentDesc = consume_argument(*CurrentDesc, Arg);
            } else {
              if (PositionalIter == PositionalList.cend()) {
                throw std::invalid_argument(
                    "positional argument exceeds maxinum consuming."s);
              }
              CurrentDesc =
                  consume_argument(ArgumentDescriptors[*PositionalIter], Arg);
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
              HelpOpt->value() = true;
            }
          }
        }
        if (HelpOpt->value()) {
          help();
          return false;
        }
        return true;
      } catch (std::exception &err) {
        std::cerr << err.what() << '\n';
        return false;
      }
    }

    void usage() const noexcept {
      using std::cout;
      cout << "\u001b[33mUSAGE\u001b[0m\n"sv;
      for (const char *Part : ProgramNames) {
        cout << '\t' << Part;
      }

      if (NonpositionalList.size() != 0) {
        cout << " [OPTIONS]"sv;
      }
      bool First = true;
      for (const auto &Index : PositionalList) {
        const auto &Desc = ArgumentDescriptors[Index];
        if (Desc.hidden()) {
          continue;
        }

        if (First) {
          cout << " [--]"sv;
          First = false;
        }

        const bool Optional = (Desc.min_nargs() == 0);
        cout << ' ';
        if (Optional) {
          cout << '[';
        }
        switch (ArgumentDescriptors[Index].max_nargs()) {
        case 0:
          break;
        case 1:
          cout << Desc.meta();
          break;
        default:
          cout << Desc.meta() << " ..."sv;
          break;
        }
        if (Optional) {
          cout << ']';
        }
      }
      cout << '\n';
    }

    void help() const noexcept {
      usage();
      using std::cout;
      const constexpr std::string_view kIndent = "\t"sv;

      cout << '\n';
      if (!SubCommandList.empty()) {
        cout << "\u001b[33mSubCommands\u001b[0m\n"sv;
        for (const auto Offset : SubCommandList) {
          cout << kIndent;
          cout << "\u001b[32m"sv;
          bool First = true;
          for (const auto &Name : this[Offset].SubCommandNames) {
            if (!First) {
              cout << '|';
            }
            cout << Name;
            First = false;
          }
          cout << "\u001b[0m\n"sv;
          indent_output(kIndent, 2, 80, this[Offset].SC->description());
          cout << '\n';
        }
        cout << '\n';
      }

      cout << "\u001b[33mOPTIONS\u001b[0m\n"sv;
      for (const auto &Index : NonpositionalList) {
        const auto &Desc = ArgumentDescriptors[Index];
        if (Desc.hidden()) {
          continue;
        }

        cout << kIndent;
        cout << "\u001b[32m"sv;
        bool First = true;
        for (const auto &Option : Desc.options()) {
          if (!First) {
            cout << '|';
          }
          if (Option.size() == 1) {
            cout << '-' << Option;
          } else {
            cout << '-' << '-' << Option;
          }
          First = false;
        }
        cout << "\u001b[0m\n"sv;
        indent_output(kIndent, 2, 80, Desc.description());
        cout << '\n';
      }
    }

    void indent_output(const std::string_view kIndent, std::size_t IndentCount,
                       std::size_t ScreenWidth,
                       std::string_view Desc) const noexcept {
      using std::cout;
      const std::size_t Width = ScreenWidth - kIndent.size() * IndentCount;
      while (Desc.size() > Width) {
        const std::size_t SpacePos = Desc.find_last_of(' ', Width);
        if (SpacePos != std::string_view::npos) {
          for (std::size_t I = 0; I < IndentCount; ++I) {
            cout << kIndent;
          }
          cout << Desc.substr(0, SpacePos) << '\n';
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
          cout << kIndent;
        }
        cout << Desc;
      }
    }

  private:
    ArgumentDescriptor *consume_short_options(std::string_view Arg) {
      ArgumentDescriptor *CurrentDesc = nullptr;
      for (std::size_t I = 1; I < Arg.size(); ++I) {
        if (CurrentDesc && CurrentDesc->nargs() == 0) {
          CurrentDesc->default_value();
        }
        std::string_view Option = Arg.substr(I, 1);
        CurrentDesc = consume_short_option(Option);
      }
      return CurrentDesc;
    }
    ArgumentDescriptor *
    consume_long_option_with_argument(std::string_view Arg) {
      if (auto Pos = Arg.find('=', 2); Pos != std::string_view::npos) {
        // long option with argument
        std::string_view Option = Arg.substr(2, Pos - 2);
        std::string_view Argument = Arg.substr(Pos + 1);
        ArgumentDescriptor *CurrentDesc = consume_long_option(Option);
        if (CurrentDesc) {
          consume_argument(*CurrentDesc, Argument);
        } else {
          throw std::invalid_argument("option "s + std::string(Option) +
                                      "doesn't need arguments."s);
        }
        return nullptr;
      } else {
        // long option without argument
        std::string_view Option = Arg.substr(2);
        return consume_long_option(Option);
      }
    }
    ArgumentDescriptor *consume_short_option(std::string_view Option) {
      auto Iter = ArgumentMap.find(Option);
      if (Iter == ArgumentMap.end()) {
        throw std::invalid_argument("unknown option: "s + std::string(Option));
      }
      ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
      if (CurrentDesc.max_nargs() == 0) {
        CurrentDesc.default_value();
        return nullptr;
      }
      return &CurrentDesc;
    }
    ArgumentDescriptor *consume_long_option(std::string_view Option) {
      auto Iter = ArgumentMap.find(Option);
      if (Iter == ArgumentMap.end()) {
        throw std::invalid_argument("unknown option: "s + std::string(Option));
      }
      ArgumentDescriptor &CurrentDesc = ArgumentDescriptors[Iter->second];
      if (CurrentDesc.max_nargs() == 0) {
        CurrentDesc.default_value();
        return nullptr;
      }
      return &CurrentDesc;
    }
    ArgumentDescriptor *consume_argument(ArgumentDescriptor &CurrentDesc,
                                         std::string_view Argument) {
      CurrentDesc.value(std::string(Argument));
      if (++CurrentDesc.nargs() >= CurrentDesc.max_nargs()) {
        return nullptr;
      }
      return &CurrentDesc;
    }

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
  };

public:
  ArgumentParser()
      : SubCommandDescriptors(1), CurrentSubCommandId(0),
        VerOpt(Description("Show version infomation"sv)) {
    SubCommandDescriptors.front().add_option("v"sv, VerOpt);
    SubCommandDescriptors.front().add_option("version"sv, VerOpt);
  }

  template <typename T>
  ArgumentParser &add_option(std::string_view Argument, T &Opt) {
    SubCommandDescriptors[CurrentSubCommandId].add_option(Argument, Opt);
    return *this;
  }

  template <typename T> ArgumentParser &add_option(T &Opt) {
    SubCommandDescriptors[CurrentSubCommandId].add_option(Opt);
    return *this;
  }

  template <typename... ArgsT>
  ArgumentParser &begin_subcommand(SubCommand &SC, ArgsT &&...Args) {
    SubCommandStack.push_back(CurrentSubCommandId);
    const auto ParentSubCommandId =
        std::exchange(CurrentSubCommandId, SubCommandDescriptors.size());
    SubCommandDescriptors.emplace_back(SC);
    SubCommandDescriptors[ParentSubCommandId].add_child(
        SubCommandDescriptors[CurrentSubCommandId],
        std::forward<ArgsT>(Args)...);
    return *this;
  }

  ArgumentParser &end_subcommand() {
    CurrentSubCommandId = SubCommandStack.back();
    SubCommandStack.pop_back();
    return *this;
  }

  bool parse(int Argc, const char *Argv[]) noexcept {
    return SubCommandDescriptors.front().parse({}, Argc, Argv, 0,
                                               VerOpt.value()) ||
           VerOpt.value();
  }
  void usage() const noexcept { SubCommandDescriptors.front().usage(); }
  void help() const noexcept { SubCommandDescriptors.front().help(); }
  bool isVersion() const noexcept { return VerOpt.value(); }

private:
  std::vector<SubCommandDescriptor> SubCommandDescriptors;
  std::size_t CurrentSubCommandId;
  std::vector<std::size_t> SubCommandStack;
  Option<Toggle> VerOpt;
};

} // namespace PO
} // namespace WasmEdge
