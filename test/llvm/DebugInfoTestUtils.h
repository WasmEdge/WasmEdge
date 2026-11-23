// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "data.h"
#include "debuginfo/debug_info.h"
#include "debuginfo/dwarf_die.h"
#include "debuginfo/dwarf_reader.h"
#include "debuginfo/type_translator.h"

#include "common/configure.h"
#include "common/defines.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <llvm-c/Core.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFContext.h>
#include <llvm/IR/DebugInfoMetadata.h>
#if LLVM_VERSION_MAJOR < 19
#else
#include <llvm/IR/DebugProgramInstruction.h>
#endif
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Object/ObjectFile.h>
#if WASMEDGE_OS_MACOS
#include <llvm/Object/MachO.h>
#endif
#include <llvm/Support/LEB128.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge::DebugInfoTest {

using namespace std::literals;

/// True when native code is big-endian. The compiler then emits only
/// functions and lines, because wasm memory is little-endian.
inline bool nativeGetsOnlyLines() noexcept {
  const uint16_t One = 1;
  uint8_t First = 0;
  std::memcpy(&First, &One, 1);
  return First == 0;
}

#define SKIP_IF_NATIVE_GETS_ONLY_LINES()                                       \
  if (nativeGetsOnlyLines()) {                                                 \
    GTEST_SKIP() << "a big-endian target gets only functions and lines";       \
  }

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

/// A DIE for writeDwarfUnit. A DW_FORM_ref4 attribute holds the Id of the
/// DIE it refers to. A string, block, or expression attribute holds its
/// bytes in Bytes.
struct TestDie {
  struct Attribute {
    llvm::dwarf::Attribute Name;
    llvm::dwarf::Form Form;
    uint64_t Value = 0;
    std::vector<uint8_t> Bytes = {};
  };
  llvm::dwarf::Tag Tag;
  std::vector<Attribute> Attributes = {};
  std::vector<TestDie> Children = {};
  uint32_t Id = 0;
};

inline TestDie::Attribute strAttr(llvm::dwarf::Attribute Name,
                                  std::string_view Text) {
  return {Name, llvm::dwarf::DW_FORM_string, 0,
          std::vector<uint8_t>(Text.begin(), Text.end())};
}

inline TestDie::Attribute refAttr(llvm::dwarf::Attribute Name, uint32_t Id) {
  return {Name, llvm::dwarf::DW_FORM_ref4, Id};
}

struct TestDwarf {
  std::vector<uint8_t> Info;
  std::vector<uint8_t> Abbrev;
  /// The .debug_info offset of each DIE with an Id.
  std::map<uint32_t, uint64_t> Offsets;
};

/// Writes one compile unit with the given DWARF version for a wasm32
/// module. Each DIE gets its own abbreviation.
inline TestDwarf writeDwarfUnit(uint16_t Version, const TestDie &Root) {
  TestDwarf Out;
  auto Fixed = [](std::vector<uint8_t> &To, uint64_t V, unsigned Size) {
    for (unsigned I = 0; I < Size; ++I) {
      To.push_back(static_cast<uint8_t>(V >> (I * 8)));
    }
  };
  auto ULEB = [](std::vector<uint8_t> &To, uint64_t V) {
    do {
      const auto B = static_cast<uint8_t>(V & 0x7f);
      V >>= 7;
      To.push_back(V ? B | 0x80 : B);
    } while (V);
  };
  auto SLEB = [](std::vector<uint8_t> &To, int64_t V) {
    while (true) {
      const auto B = static_cast<uint8_t>(V & 0x7f);
      V >>= 7;
      if ((V == 0 && !(B & 0x40)) || (V == -1 && (B & 0x40))) {
        To.push_back(B);
        return;
      }
      To.push_back(B | 0x80);
    }
  };
  auto &Info = Out.Info;
  Fixed(Info, 0, 4);
  Fixed(Info, Version, 2);
  if (Version < 5) {
    Fixed(Info, 0, 4);
    Info.push_back(4);
  } else {
    Info.push_back(llvm::dwarf::DW_UT_compile);
    Info.push_back(4);
    Fixed(Info, 0, 4);
  }
  std::vector<std::pair<size_t, uint32_t>> Refs;
  uint64_t Code = 0;
  std::function<void(const TestDie &)> Write = [&](const TestDie &D) {
    ULEB(Out.Abbrev, ++Code);
    ULEB(Out.Abbrev, D.Tag);
    Out.Abbrev.push_back(D.Children.empty() ? 0 : 1);
    if (D.Id) {
      Out.Offsets[D.Id] = Info.size();
    }
    ULEB(Info, Code);
    for (const auto &A : D.Attributes) {
      ULEB(Out.Abbrev, A.Name);
      ULEB(Out.Abbrev, A.Form);
      switch (A.Form) {
      case llvm::dwarf::DW_FORM_data1:
        Fixed(Info, A.Value, 1);
        break;
      case llvm::dwarf::DW_FORM_data2:
        Fixed(Info, A.Value, 2);
        break;
      case llvm::dwarf::DW_FORM_data4:
      case llvm::dwarf::DW_FORM_sec_offset:
        Fixed(Info, A.Value, 4);
        break;
      case llvm::dwarf::DW_FORM_data8:
        Fixed(Info, A.Value, 8);
        break;
      case llvm::dwarf::DW_FORM_udata:
        ULEB(Info, A.Value);
        break;
      case llvm::dwarf::DW_FORM_sdata:
        SLEB(Info, static_cast<int64_t>(A.Value));
        break;
      case llvm::dwarf::DW_FORM_ref4:
        Refs.emplace_back(Info.size(), static_cast<uint32_t>(A.Value));
        Fixed(Info, 0, 4);
        break;
      case llvm::dwarf::DW_FORM_string:
        Info.insert(Info.end(), A.Bytes.begin(), A.Bytes.end());
        Info.push_back(0);
        break;
      case llvm::dwarf::DW_FORM_exprloc:
      case llvm::dwarf::DW_FORM_block:
        ULEB(Info, A.Bytes.size());
        Info.insert(Info.end(), A.Bytes.begin(), A.Bytes.end());
        break;
      case llvm::dwarf::DW_FORM_flag_present:
        break;
      default:
        ADD_FAILURE() << "writeDwarfUnit: unsupported form " << A.Form;
        break;
      }
    }
    Out.Abbrev.push_back(0);
    Out.Abbrev.push_back(0);
    if (!D.Children.empty()) {
      for (const auto &C : D.Children) {
        Write(C);
      }
      Info.push_back(0);
    }
  };
  Write(Root);
  Out.Abbrev.push_back(0);
  for (const auto &[At, Id] : Refs) {
    const auto It = Out.Offsets.find(Id);
    EXPECT_NE(It, Out.Offsets.end()) << "writeDwarfUnit: no DIE " << Id;
    if (It != Out.Offsets.end()) {
      for (unsigned I = 0; I < 4; ++I) {
        Info[At + I] = static_cast<uint8_t>(It->second >> (I * 8));
      }
    }
  }
  const auto Length = Info.size() - 4;
  for (unsigned I = 0; I < 4; ++I) {
    Info[I] = static_cast<uint8_t>(Length >> (I * 8));
  }
  return Out;
}

/// A module with functions and no DWARF, which gets the sections of Dwarf
/// and the Extra sections.
inline std::unique_ptr<AST::Module>
moduleWithDwarf(const Configure &Conf, const TestDwarf &Dwarf,
                const std::vector<std::pair<std::string, std::vector<uint8_t>>>
                    &Extra = {}) {
  auto Mod = loadFixture(Conf, "dwarf_basic_nodebug.wasm"sv);
  if (!Mod) {
    return nullptr;
  }
  for (const auto &[Name, Content] :
       {std::pair{".debug_info"sv, &Dwarf.Info},
        std::pair{".debug_abbrev"sv, &Dwarf.Abbrev},
        std::pair{".debug_line"sv,
                  static_cast<const std::vector<uint8_t> *>(nullptr)}}) {
    auto &Custom = Mod->getCustomSections().emplace_back();
    Custom.setName(Name);
    if (Content) {
      Custom.getContent() = *Content;
    }
  }
  for (const auto &[Name, Content] : Extra) {
    auto &Custom = Mod->getCustomSections().emplace_back();
    Custom.setName(Name);
    Custom.getContent() = Content;
  }
  return Mod;
}

inline llvm::DWARFDie findType(llvm::DWARFContext &Ctx, llvm::dwarf::Tag Tag,
                               std::string_view Name) {
  for (const auto &CU : Ctx.compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() == Tag && Die.getName(llvm::DINameKind::ShortName) &&
          Name == Die.getName(llvm::DINameKind::ShortName)) {
        return Die;
      }
    }
  }
  return {};
}

} // namespace WasmEdge::DebugInfoTest
