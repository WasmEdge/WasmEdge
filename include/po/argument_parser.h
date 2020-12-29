// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/argument_parser.h - Argument parser -----------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "po/list.h"
#include "po/option.h"

#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SSVM {
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

public:
  ArgumentParser()
      : HelpOpt(Description("Show this help messages"sv)),
        VerOpt(Description("Show version infomation"sv)) {
    add_option("h"sv, HelpOpt);
    add_option("help"sv, HelpOpt);
    add_option("v"sv, VerOpt);
    add_option("version"sv, VerOpt);
  }

  template <typename T>
  ArgumentParser &add_option(std::string_view Argument, T &Opt) {
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
    return *this;
  }

  template <typename T> ArgumentParser &add_option(T &Opt) {
    if (auto Iter = OptionMap.find(std::addressof(Opt));
        Iter == OptionMap.end()) {
      OptionMap.emplace(std::addressof(Opt), ArgumentDescriptors.size());
      PositionalList.emplace_back(ArgumentDescriptors.size());
      ArgumentDescriptors.emplace_back(Opt);
    } else {
      PositionalList.emplace_back(Iter->second);
    }
    return *this;
  }

  bool parse(int Argc, const char *Argv[]) noexcept {
    try {
      ProgramName = Argv[0];
      ArgumentDescriptor *CurrentDesc = nullptr;
      bool Escaped = false;
      auto PositionalIter = PositionalList.cbegin();
      for (std::string_view Arg : Span<const char *>(Argv + 1, Argc - 1)) {
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

      if (VerOpt.value()) {
        return true;
      }
      for (const auto &Desc : ArgumentDescriptors) {
        if (Desc.nargs() < Desc.min_nargs()) {
          HelpOpt.value() = true;
        }
      }
      if (HelpOpt.value()) {
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
    cout << "usage: "sv << ProgramName;
    for (const auto &Index : NonpositionalList) {
      const auto &Desc = ArgumentDescriptors[Index];
      if (Desc.hidden()) {
        continue;
      }

      bool First = true;
      cout << ' ' << '[';
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
      switch (Desc.max_nargs()) {
      case 0:
        break;
      case 1:
        cout << ' ' << Desc.meta();
        break;
      default:
        cout << ' ' << Desc.meta() << " ..."sv;
        break;
      }
      cout << ']';
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
    const constexpr std::string_view kIndent = "  "sv;

    cout << "Options:\n"sv;
    for (const auto &Index : NonpositionalList) {
      const auto &Desc = ArgumentDescriptors[Index];
      if (Desc.hidden()) {
        continue;
      }

      cout << kIndent;
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
      cout << '\n';
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
  bool isHelp() const noexcept { return HelpOpt.value(); }
  bool isVersion() const noexcept { return VerOpt.value(); }

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
  ArgumentDescriptor *consume_long_option_with_argument(std::string_view Arg) {
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

  std::string ProgramName;
  std::vector<ArgumentDescriptor> ArgumentDescriptors;
  std::unordered_map<void *, std::size_t> OptionMap;
  std::unordered_map<std::string_view, std::size_t> ArgumentMap;
  std::vector<std::size_t> NonpositionalList;
  std::vector<std::size_t> PositionalList;
  Option<Toggle> HelpOpt;
  Option<Toggle> VerOpt;
};

} // namespace PO
} // namespace SSVM
