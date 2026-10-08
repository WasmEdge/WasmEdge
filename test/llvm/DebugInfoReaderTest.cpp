// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "DebugInfoTestUtils.h"

namespace {

using namespace WasmEdge;
using namespace WasmEdge::DebugInfoTest;
using namespace std::literals;

uint32_t lineOf(const llvm::DILineInfo &Info) { return Info.Line; }

template <typename T> uint32_t lineOf(const std::optional<T> &Info) {
  return Info ? lineOf(*Info) : 0U;
}

TEST(DwarfReader, NoDebugSectionsGivesNull) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_nodebug.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  EXPECT_EQ(LLVM::DebugInfo::DwarfReader::create(*Mod), nullptr);
}

TEST(DwarfReader, MatchesEveryNamedFunction) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  EXPECT_EQ(R->getErrorCount(), 0U);
  std::vector<std::string> Names;
  for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
    if (auto Die = R->getSubprogram(I)) {
      Names.emplace_back(Die.getName(llvm::DINameKind::ShortName));
    }
  }
  EXPECT_NE(std::find(Names.begin(), Names.end(), "sumList"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "fact"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "run"), Names.end());
}

uint32_t renameSubprogramLowPC(AST::Module &Mod) {
  return renameAbbrevAttribute(
      Mod,
      [](uint64_t, uint64_t Tag) {
        return Tag == llvm::dwarf::DW_TAG_subprogram;
      },
      llvm::dwarf::DW_AT_low_pc, llvm::dwarf::DW_AT_entry_pc);
}

std::vector<std::string> moveSubprogramLowPC(
    AST::Module &Mod,
    const std::function<uint64_t(const AST::CodeSegment &,
                                 const LLVM::DebugInfo::DwarfReader &)> &Pick) {
  std::vector<std::pair<uint64_t, uint32_t>> Patches;
  std::vector<std::string> Names;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(Mod);
    if (!R) {
      return {};
    }
    const auto Segs = Mod.getCodeSection().getContent();
    Names.resize(Segs.size());
    for (uint32_t I = 0; I < Segs.size(); ++I) {
      auto Die = R->getSubprogram(I);
      if (!Die) {
        continue;
      }
      Names[I] = Die.getName(llvm::DINameKind::ShortName);
      const auto OldLow =
          llvm::dwarf::toAddress(Die.find(llvm::dwarf::DW_AT_low_pc));
      const auto NewLow = static_cast<uint32_t>(Pick(Segs[I], *R));
      for (const auto &A : Die.attributes()) {
        if (A.Attr == llvm::dwarf::DW_AT_low_pc && A.ByteSize == 4) {
          Patches.emplace_back(A.Offset, NewLow);
        } else if (A.Attr == llvm::dwarf::DW_AT_high_pc && A.ByteSize == 4 &&
                   A.Value.getForm() == llvm::dwarf::DW_FORM_data4 && OldLow) {
          const auto Size = A.Value.getAsUnsignedConstant();
          Patches.emplace_back(
              A.Offset,
              static_cast<uint32_t>(*OldLow + (Size ? *Size : 0) - NewLow));
        }
      }
    }
  }
  for (auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != ".debug_info"sv) {
      continue;
    }
    auto &Bytes = Custom.getContent();
    for (const auto &[Offset, Value] : Patches) {
      for (uint32_t I = 0; I < 4; ++I) {
        Bytes[Offset + I] = static_cast<uint8_t>(Value >> (I * 8));
      }
    }
  }
  return Patches.empty() ? std::vector<std::string>{} : Names;
}

TEST(DwarfReader, MatchesSubprogramByEntryPC) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  ASSERT_GT(renameSubprogramLowPC(*Mod), 0U);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  std::vector<std::string> Names;
  for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
    if (auto Die = R->getSubprogram(I)) {
      EXPECT_FALSE(Die.find(llvm::dwarf::DW_AT_low_pc));
      Names.emplace_back(Die.getName(llvm::DINameKind::ShortName));
    }
  }
  EXPECT_NE(std::find(Names.begin(), Names.end(), "sumList"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "fact"), Names.end());
  EXPECT_NE(std::find(Names.begin(), Names.end(), "run"), Names.end());
}

TEST(DwarfReader, MatchesSubprogramAtFirstInstruction) {
  enum class Form { LowPC, EntryPC, Range };
  for (const auto F : {Form::LowPC, Form::EntryPC, Form::Range}) {
    Configure Conf;
    auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
    ASSERT_NE(Mod, nullptr);
    const auto Want =
        moveSubprogramLowPC(*Mod, [F](const AST::CodeSegment &Seg,
                                      const LLVM::DebugInfo::DwarfReader &R) {
          const auto First =
              R.toAddress(Seg.getExpr().getInstrs().front().getOffset());
          return F == Form::Range ? First - 1 : First;
        });
    ASSERT_FALSE(Want.empty());
    if (F == Form::EntryPC) {
      ASSERT_GT(renameSubprogramLowPC(*Mod), 0U);
    }
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    for (uint32_t I = 0; I < Want.size(); ++I) {
      auto Die = R->getSubprogram(I);
      EXPECT_EQ(Die ? Die.getName(llvm::DINameKind::ShortName) : "", Want[I])
          << "form " << static_cast<int>(F) << " function " << I;
    }
  }
}

TEST(DwarfReader, SharedLowPCDoesNotPickTheFirstSubprogram) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  std::vector<uint32_t> Matched;
  std::vector<uint64_t> Lows;
  {
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
      if (auto Die = R->getSubprogram(I)) {
        Matched.push_back(I);
        Lows.push_back(
            llvm::dwarf::toAddress(Die.find(llvm::dwarf::DW_AT_low_pc), 0));
      }
    }
  }
  ASSERT_GE(Matched.size(), 2U);
  uint32_t Seen = 0;
  const auto Names =
      moveSubprogramLowPC(*Mod, [&](const AST::CodeSegment &,
                                    const LLVM::DebugInfo::DwarfReader &) {
        const auto K = Seen++;
        return K == 0 ? Lows[1] : Lows[K];
      });
  ASSERT_FALSE(Names.empty());
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  auto Second = R->getSubprogram(Matched[1]);
  EXPECT_NE(Second ? Second.getName(llvm::DINameKind::ShortName) : "",
            Names[Matched[0]]);
}

TEST(DwarfReader, LineForFirstInstructionOfSumList) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  const auto Segs = Mod->getCodeSection().getContent();
  for (uint32_t I = 0; I < Segs.size(); ++I) {
    auto Die = R->getSubprogram(I);
    if (!Die || Die.getName(llvm::DINameKind::ShortName) != "sumList"sv) {
      continue;
    }
    const auto Addr =
        R->toAddress(Segs[I].getExpr().getInstrs().front().getOffset());
    auto Info = R->getContext().getLineInfoForAddress(
        {Addr, llvm::object::SectionedAddress::UndefSection});
    EXPECT_EQ(lineOf(Info), Die.getDeclLine());
    return;
  }
  FAIL() << "sumList not found";
}

TEST(DwarfReader, CorruptDebugInfoIsCounted) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  for (auto &Custom : Mod->getCustomSections()) {
    if (Custom.getName() == ".debug_info"sv) {
      auto &Bytes = Custom.getContent();
      Bytes.resize(Bytes.size() / 2);
    }
  }
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  for (uint32_t I = 0; I < Mod->getCodeSection().getContent().size(); ++I) {
    (void)R->getSubprogram(I);
  }
  EXPECT_GT(R->getErrorCount(), 0U);
}

TEST(DwarfReader, FallbackFunctionName) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  EXPECT_EQ(R->getFunctionName(100000U), "func[100000]"s);
}

TEST(DwarfReader, NameSectionUsesWasmIntegerAndNameRules) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  const std::vector<Byte> WideIndex = {0x01, 0x0c, 0x02, 0x00, 0x02, 'o',  'k',
                                       0x81, 0x80, 0x80, 0x80, 0x10, 0x01, 'x'};
  const std::vector<Byte> BadUtf8 = {0x01, 0x08, 0x02, 0x00, 0x02,
                                     'o',  'k',  0x01, 0x01, 0xff};
  for (const auto &Bytes : {WideIndex, BadUtf8}) {
    for (auto &Custom : Mod->getCustomSections()) {
      if (Custom.getName() == "name"sv) {
        Custom.getContent() = Bytes;
      }
    }
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    EXPECT_EQ(R->getFunctionName(0), "ok"s);
    EXPECT_EQ(R->getFunctionName(1), "func[1]"s);
    EXPECT_GT(R->getErrorCount(), 0U);
  }
}

TEST(DwarfReader, NameSectionRecovery) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_basic_O0.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  const std::vector<Byte> RepeatedIndex = {0x01, 0x08, 0x02, 0x00, 0x02,
                                           'o',  'k',  0x00, 0x01, 'x'};
  const std::vector<Byte> TrailingByte = {0x01, 0x06, 0x01, 0x00,
                                          0x02, 'o',  'k',  0x00};
  for (const auto *Bytes : {&RepeatedIndex, &TrailingByte}) {
    for (auto &Custom : Mod->getCustomSections()) {
      if (Custom.getName() == "name"sv) {
        Custom.getContent() = *Bytes;
      }
    }
    auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    EXPECT_EQ(R->getFunctionName(0), "ok"s);
    EXPECT_EQ(R->getErrorCount() > 0, Bytes == &TrailingByte);
  }
}

} // namespace
