// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/dwarf_reader.h"

#include "common/configure.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge::DebugInfoTest {

using namespace std::literals;

inline std::filesystem::path fixture(std::string_view Name) {
  return std::filesystem::u8path(WASMEDGE_DEBUGINFO_FIXTURES) /
         std::filesystem::u8path(Name);
}

inline std::vector<uint8_t> readHexFixture(std::string_view Name) {
  std::ifstream In(fixture(std::string(Name) + ".hex"), std::ios::binary);
  EXPECT_TRUE(In.is_open()) << Name;
  std::vector<uint8_t> Bytes;
  int Pending = -1;
  char C;
  while (In.get(C)) {
    if (C == ' ' || C == '\t' || C == '\r' || C == '\n') {
      continue;
    }
    int V = 0;
    if (C >= '0' && C <= '9') {
      V = C - '0';
    } else if (C >= 'a' && C <= 'f') {
      V = C - 'a' + 10;
    } else if (C >= 'A' && C <= 'F') {
      V = C - 'A' + 10;
    } else {
      ADD_FAILURE() << Name << ": bad hex character " << static_cast<int>(C);
      return {};
    }
    if (Pending < 0) {
      Pending = V;
    } else {
      Bytes.push_back(static_cast<uint8_t>(Pending * 16 + V));
      Pending = -1;
    }
  }
  if (Pending >= 0) {
    ADD_FAILURE() << Name << ": odd number of hex digits";
    return {};
  }
  return Bytes;
}

inline std::unique_ptr<AST::Module> loadFixture(const Configure &Conf,
                                                std::string_view Name) {
  auto Bytes = readHexFixture(Name);
  if (Bytes.empty()) {
    return nullptr;
  }
  Loader::Loader Ldr(Conf, &Executor::Executor::Intrinsics);
  auto Mod = Ldr.parseModule(Bytes);
  if (!Mod) {
    return nullptr;
  }
  Validator::Validator Valid(Conf);
  if (!Valid.validate(**Mod)) {
    return nullptr;
  }
  return std::move(*Mod);
}

/// Calls Edit for each attribute specification of the abbreviations that
/// Match accepts, with the positions of its attribute and form codes.
inline uint32_t editAbbrevs(
    AST::Module &Mod,
    const std::function<bool(uint64_t Code, uint64_t Tag, uint64_t Attr)>
        &Match,
    const std::function<void(std::vector<Byte> &, size_t AttrAt, size_t FormAt)>
        &Edit) {
  uint32_t Patched = 0;
  for (auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != ".debug_abbrev"sv) {
      continue;
    }
    auto &Bytes = Custom.getContent();
    size_t I = 0;
    auto uleb = [&]() {
      uint64_t Value = 0;
      for (uint32_t Shift = 0; I < Bytes.size(); Shift += 7) {
        const auto B = Bytes[I++];
        Value |= static_cast<uint64_t>(B & 0x7f) << Shift;
        if (!(B & 0x80)) {
          break;
        }
      }
      return Value;
    };
    while (I < Bytes.size()) {
      const auto Code = uleb();
      if (Code == 0) {
        break;
      }
      const auto Tag = uleb();
      ++I;
      while (true) {
        const size_t AttrAt = I;
        const auto Attr = uleb();
        const size_t FormAt = I;
        const auto Form = uleb();
        if (Attr == 0 && Form == 0) {
          break;
        }
        if (Form == llvm::dwarf::DW_FORM_implicit_const) {
          uleb();
        }
        if (Match(Code, Tag, Attr)) {
          Edit(Bytes, AttrAt, FormAt);
          ++Patched;
        }
      }
    }
  }
  return Patched;
}

inline uint32_t
renameAbbrevAttribute(AST::Module &Mod,
                      const std::function<bool(uint64_t, uint64_t)> &Match,
                      llvm::dwarf::Attribute From, llvm::dwarf::Attribute To) {
  return editAbbrevs(
      Mod,
      [&](uint64_t Code, uint64_t Tag, uint64_t Attr) {
        return Attr == From && Match(Code, Tag);
      },
      [To](std::vector<Byte> &Bytes, size_t AttrAt, size_t) {
        Bytes[AttrAt] = static_cast<Byte>(To);
      });
}

} // namespace WasmEdge::DebugInfoTest
