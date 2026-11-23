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

TEST(DwarfReader, StackValueDropsMemoryLocations) {
  using LLVM::DebugInfo::decodeLocation;
  using Kind = LLVM::DebugInfo::WasmLocation::Kind;
  const uint8_t FrameBase[] = {0x91, 0x0c};
  const uint8_t FrameBaseValue[] = {0x91, 0x0c, 0x9f};
  const uint8_t Addr[] = {0x03, 0x10, 0x00, 0x00, 0x00};
  const uint8_t AddrValue[] = {0x03, 0x10, 0x00, 0x00, 0x00, 0x9f};
  const uint8_t LocalValue[] = {0xed, 0x00, 0x02, 0x9f};
  const auto FB = decodeLocation(FrameBase, nullptr);
  EXPECT_EQ(FB.K, Kind::FrameBaseOffset);
  EXPECT_EQ(FB.Offset, 12);
  EXPECT_EQ(decodeLocation(FrameBaseValue, nullptr).K, Kind::None);
  const auto A = decodeLocation(Addr, nullptr);
  EXPECT_EQ(A.K, Kind::Address);
  EXPECT_EQ(A.Index, 16U);
  EXPECT_EQ(decodeLocation(AddrValue, nullptr).K, Kind::None);
  const auto L = decodeLocation(LocalValue, nullptr);
  EXPECT_EQ(L.K, Kind::Local);
  EXPECT_EQ(L.Index, 2U);
}

TEST(DwarfReader, WasmLocationOperands) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const uint8_t GlobalU32[] = {0xed, 0x03, 0x01, 0x00, 0x00, 0x00, 0x9f};
  const uint8_t Local64[] = {0xed, 0x00, 0x40};
  const auto G = decodeLocation(GlobalU32, nullptr);
  EXPECT_EQ(G.K, Kind::Global);
  EXPECT_EQ(G.Index, 1U);
  EXPECT_FALSE(G.Indirect);
  const auto L = decodeLocation(Local64, nullptr, WasmLocationConvention::LLVM);
  EXPECT_EQ(L.K, Kind::Local);
  EXPECT_EQ(L.Index, 64U);
  EXPECT_TRUE(L.Indirect);
  const auto S =
      decodeLocation(Local64, nullptr, WasmLocationConvention::Standard);
  EXPECT_EQ(S.K, Kind::Local);
  EXPECT_EQ(S.Index, 64U);
  EXPECT_FALSE(S.Indirect);
  EXPECT_EQ(decodeLocation(Local64, nullptr).K, Kind::None);
}

TEST(DwarfReader, LocationConventionFollowsTheProducer) {
  namespace dwarf = llvm::dwarf;
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const std::tuple<const char *, WasmLocationConvention, Kind, bool> Cases[] = {
      {"clang version 21.1.8", WasmLocationConvention::LLVM, Kind::Local, true},
      {"clang LLVM (rustc version 1.98.1)", WasmLocationConvention::LLVM,
       Kind::Local, true},
      {"Example Wasm Compiler 1.0", WasmLocationConvention::Standard,
       Kind::Local, false},
      {nullptr, WasmLocationConvention::Unknown, Kind::None, false}};
  for (const auto &[Producer, Convention, K, Indirect] : Cases) {
    TestDie Unit{
        dwarf::DW_TAG_compile_unit,
        {{dwarf::DW_AT_language, dwarf::DW_FORM_data2, dwarf::DW_LANG_C11}}};
    if (Producer) {
      Unit.Attributes.push_back(strAttr(dwarf::DW_AT_producer, Producer));
    }
    Unit.Children.push_back({dwarf::DW_TAG_variable,
                             {strAttr(dwarf::DW_AT_name, "v"),
                              {dwarf::DW_AT_location,
                               dwarf::DW_FORM_exprloc,
                               0,
                               {0xed, 0x00, 0x02}}},
                             {},
                             1});
    const auto Dwarf = writeDwarfUnit(4, Unit);
    Configure Conf;
    auto Mod = moduleWithDwarf(Conf, Dwarf);
    ASSERT_NE(Mod, nullptr);
    auto R = DwarfReader::create(*Mod);
    ASSERT_NE(R, nullptr);
    auto Var = R->getContext().getDIEForOffset(Dwarf.Offsets.at(1));
    ASSERT_TRUE(Var);
    auto *U = Var.getDwarfUnit();
    EXPECT_EQ(locationConvention(U), Convention);
    const auto Block = dwarf::toBlock(Var.find(dwarf::DW_AT_location));
    ASSERT_TRUE(Block);
    const auto L = decodeLocation(*Block, U);
    EXPECT_EQ(L.K, K);
    EXPECT_EQ(L.Indirect, Indirect);
    if (K != Kind::None) {
      EXPECT_EQ(L.Index, 2U);
    }
  }
}

TEST(DwarfReader, LocationExpressionGrammar) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  auto decode = [](llvm::ArrayRef<uint8_t> Bytes) {
    return decodeLocation(Bytes, nullptr, WasmLocationConvention::LLVM);
  };
  const uint8_t TwoLocations[] = {0x91, 0x08, 0x03, 0x10, 0x00, 0x00, 0x00};
  const uint8_t TruncatedAddr[] = {0x03, 0x10, 0x00};
  const uint8_t TruncatedLEB[] = {0x91, 0x88};
  const uint8_t TruncatedGlobal[] = {0xed, 0x03, 0x01, 0x00};
  const uint8_t OperandStack[] = {0xed, 0x02, 0x00};
  const uint8_t AfterStackValue[] = {0xed, 0x00, 0x01, 0x9f, 0x9f};
  const uint8_t StackValueOnly[] = {0x9f};
  const uint8_t LocalValue[] = {0xed, 0x00, 0x01, 0x9f};
  const uint8_t MultiByteKind[] = {0xed, 0x80, 0x00, 0x01};
  const uint8_t LocalOver32Bits[] = {0xed, 0x00, 0x80, 0x80, 0x80, 0x80, 0x10};
  const uint8_t GlobalOver32Bits[] = {0xed, 0x01, 0x80, 0x80, 0x80, 0x80, 0x10};
  const uint8_t LargestLocal[] = {0xed, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0f};
  EXPECT_EQ(decode(TwoLocations).K, Kind::None);
  EXPECT_EQ(decode(TruncatedAddr).K, Kind::None);
  EXPECT_EQ(decode(TruncatedLEB).K, Kind::None);
  EXPECT_EQ(decode(TruncatedGlobal).K, Kind::None);
  EXPECT_EQ(decode(OperandStack).K, Kind::None);
  EXPECT_EQ(decode(AfterStackValue).K, Kind::None);
  EXPECT_EQ(decode(StackValueOnly).K, Kind::None);
  EXPECT_EQ(decode(LocalValue).K, Kind::Local);
  EXPECT_EQ(decode(MultiByteKind).K, Kind::None);
  EXPECT_EQ(decode(LocalOver32Bits).K, Kind::None);
  EXPECT_EQ(decode(GlobalOver32Bits).K, Kind::None);
  EXPECT_EQ(decode(LargestLocal).Index, UINT32_MAX);
}

TEST(DwarfReader, DefaultLocationOnlyFillsGaps) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const WasmLocation Default{Kind::Local, 1, 0, false};
  const WasmLocation Explicit{Kind::Local, 2, 0, false};
  const WasmLocation Unavailable{};
  const auto Entries = normalizeLocationEntries(
      {{0x10, 0x20, Explicit}, {0x20, 0x30, Unavailable}, {0x40, 0x40, {}}},
      {Default});
  ASSERT_EQ(Entries.size(), 4U);
  for (size_t I = 1; I < Entries.size(); ++I) {
    EXPECT_EQ(Entries[I - 1].High, Entries[I].Low);
  }
  auto At = [&Entries](uint64_t Address) {
    const auto I = findLocationEntry(Entries, Address);
    return I ? Entries[*I].L : WasmLocation{Kind::Address, 0, 0, false};
  };
  EXPECT_EQ(At(0x18).Index, 2U);
  EXPECT_EQ(At(0x28).K, Kind::None);
  EXPECT_EQ(At(0x08).Index, 1U);
  EXPECT_EQ(At(0x38).Index, 1U);
}

TEST(DwarfReader, SupportedEntryWinsOverlap) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const WasmLocation Default{Kind::Local, 1, 0, false};
  const WasmLocation Supported{Kind::Local, 2, 0, false};
  const auto Entries = normalizeLocationEntries(
      {{0x10, 0x30, {}}, {0x20, 0x40, Supported}}, {Default});
  auto At = [&Entries](uint64_t Address) {
    const auto I = findLocationEntry(Entries, Address);
    return I ? Entries[*I].L : WasmLocation{Kind::Address, 0, 0, false};
  };
  EXPECT_EQ(At(0x18).K, Kind::None);
  EXPECT_EQ(At(0x28).Index, 2U);
  EXPECT_EQ(At(0x38).Index, 2U);
  EXPECT_EQ(At(0x48).Index, 1U);
}

TEST(DwarfReader, LocalValueWinsOverlap) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const WasmLocation Frame{Kind::FrameBaseOffset, 0, 16, false};
  const WasmLocation Local{Kind::Local, 2, 0, false};
  const WasmLocation Default{Kind::Local, 1, 0, false};
  const auto Entries = normalizeLocationEntries(
      {{0x10, 0x30, Frame}, {0x20, 0x40, Local}, {0x30, 0x50, {}}}, {Default});
  auto At = [&Entries](uint64_t Address) {
    const auto I = findLocationEntry(Entries, Address);
    return I ? Entries[*I].L : WasmLocation{Kind::Address, 0, 0, false};
  };
  EXPECT_EQ(At(0x18).K, Kind::FrameBaseOffset);
  EXPECT_EQ(At(0x28).K, Kind::Local);
  EXPECT_EQ(At(0x28).Index, 2U);
  EXPECT_EQ(At(0x38).Index, 2U);
  EXPECT_EQ(At(0x48).K, Kind::None);
  EXPECT_EQ(At(0x58).Index, 1U);
}

TEST(DwarfReader, UsableConstantWinsOverlap) {
  using namespace LLVM::DebugInfo;
  using Kind = WasmLocation::Kind;
  const WasmLocation Frame{Kind::FrameBaseOffset, 0, 16, false};
  const WasmLocation Constant{Kind::Constant, 5, 0, false};
  const WasmLocation Local{Kind::Local, 2, 0, false};
  for (const bool Usable : {false, true}) {
    const auto Entries = normalizeLocationEntries(
        {{0x10, 0x20, Frame}, {0x10, 0x20, Constant}, {0x20, 0x30, Local}}, {},
        Usable);
    ASSERT_EQ(Entries.size(), 2U) << Usable;
    EXPECT_EQ(Entries[0].L.K, Usable ? Kind::Constant : Kind::FrameBaseOffset)
        << Usable;
    EXPECT_EQ(Entries[1].L.K, Kind::Local) << Usable;
  }
}

TEST(DwarfReader, LocationMustCoverTheWholeScope) {
  using namespace LLVM::DebugInfo;
  const WasmLocation FB{WasmLocation::Kind::FrameBaseOffset, 0, 8, false};
  EXPECT_TRUE(coversRange({{0x10, 0x30, FB}, {0x30, 0x40, FB}}, 0x10, 0x40));
  EXPECT_TRUE(coversRange({{0x10, 0x40, FB}}, 0x18, 0x40));
  EXPECT_FALSE(coversRange({{0x10, 0x20, FB}, {0x30, 0x40, FB}}, 0x10, 0x40));
  EXPECT_FALSE(coversRange({{0x18, 0x38, FB}}, 0x10, 0x40));
  EXPECT_FALSE(coversRange({{0x10, 0x38, FB}}, 0x10, 0x40));
  EXPECT_TRUE(coversRange({{0, UINT64_MAX, FB}}, 0x10, 0x40));
}

TEST(DwarfReader, DeclFileUsesTheUnitOfTheAttribute) {
  Configure Conf;
  auto Mod = loadFixture(Conf, "dwarf_units_O2.wasm"sv);
  ASSERT_NE(Mod, nullptr);
  auto R = LLVM::DebugInfo::DwarfReader::create(*Mod);
  ASSERT_NE(R, nullptr);
  uint32_t Checked = 0;
  for (const auto &CU : R->getContext().compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie D(CU.get(), &Entry);
      auto Origin = D.getAttributeValueAsReferencedDie(
          llvm::dwarf::DW_AT_abstract_origin);
      if (D.getTag() != llvm::dwarf::DW_TAG_variable || !Origin ||
          Origin.getDwarfUnit() == CU.get()) {
        continue;
      }
      const auto Path = LLVM::DebugInfo::declFile(D);
      EXPECT_TRUE(Path.size() >= "dwarf_units_helper.c"sv.size() &&
                  Path.compare(Path.size() - "dwarf_units_helper.c"sv.size(),
                               std::string::npos, "dwarf_units_helper.c") == 0)
          << Path;
      ++Checked;
    }
  }
  EXPECT_GT(Checked, 0U);
}

TEST(DwarfReader, ArrayBoundForms) {
  using LLVM::DebugInfo::defaultLowerBound;
  using LLVM::DebugInfo::integerConstant;
  namespace dwarf = llvm::dwarf;
  const auto SData =
      llvm::DWARFFormValue::createFromSValue(dwarf::DW_FORM_sdata, -3);
  const auto Data1 =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_data1, 0xfd);
  const auto Ref =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_ref4, 8);
  const auto Data8 =
      llvm::DWARFFormValue::createFromUValue(dwarf::DW_FORM_data8, UINT64_MAX);
  auto Value = [](const llvm::DWARFFormValue &V, bool Signed) {
    const auto C = integerConstant(V, Signed);
    return C ? C->toInt64() : std::nullopt;
  };
  EXPECT_EQ(Value(SData, false), -3);
  EXPECT_EQ(Value(Data1, true), -3);
  EXPECT_EQ(Value(Data1, false), 0xfd);
  EXPECT_FALSE(integerConstant(Ref, false));
  EXPECT_EQ(Value(Data8, true), -1);
  const auto Big = integerConstant(Data8, false);
  ASSERT_TRUE(Big);
  EXPECT_FALSE(Big->toInt64());
  EXPECT_EQ(Big->bits(), UINT64_MAX);
  EXPECT_EQ(defaultLowerBound(dwarf::DW_LANG_C11), 0);
  EXPECT_EQ(defaultLowerBound(dwarf::DW_LANG_Fortran90), 1);
}

} // namespace
