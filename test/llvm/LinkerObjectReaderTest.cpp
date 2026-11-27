// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include <llvm/Support/JSON.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "LinkerTestHelpers.h"
#include "LinkerTestUtils.h"
#include "linker/compact_unwind.h"
#include "linker/eh_frame.h"
#include "linker/elf_writer.h"
#include "linker/layout.h"
#include "linker/link_graph.h"
#include "linker/native_linker.h"
#include "linker/object_reader.h"
#include "linker/relocation.h"
#include "linker/universal_wasm_writer.h"
#include "linker/writer.h"

#include "aot/version.h"

#include <gtest/gtest.h>

#include <llvm/BinaryFormat/COFF.h>
#include <llvm/BinaryFormat/MachO.h>
#include <llvm/Config/llvm-config.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#if LLVM_VERSION_MAJOR < 19
#else
#include <llvm/MC/MCELFExtras.h>
#endif
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/MachO.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/ARMBuildAttributes.h>
#include <llvm/Support/raw_ostream.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace WasmEdge::LLVM::Linker;
using namespace WasmEdge::LLVM::Test;

struct LinuxRelocationCase {
  const char *Triple;
  Target Architecture;
  const char *TunedCPU;
  const char *Features;
  std::set<uint64_t> SupportedTypes;
  std::set<uint64_t> RequiredTypes;
};

struct PortableObjectCase {
  const char *Triple;
  Target Architecture;
  std::set<uint32_t> Types;
};

[[maybe_unused]] std::string linuxRelocationCaseName(
    const testing::TestParamInfo<LinuxRelocationCase> &Info) {
  return architectureName(Info.param.Architecture);
}

[[maybe_unused]] std::string
portableObjectCaseName(const testing::TestParamInfo<PortableObjectCase> &Info) {
  return architectureName(Info.param.Architecture);
}

std::optional<uint8_t> expectedELFPatchWidth(Target Architecture,
                                             uint32_t Type) {
  constexpr uint8_t NoBytes = 0;
  constexpr uint8_t ByteBytes = 1;
  constexpr uint8_t WordBytes = 4;
  constexpr uint8_t DoubleWordBytes = 8;
  switch (Architecture) {
  case Target::ARM:
    switch (Type) {
    case llvm::ELF::R_ARM_NONE:
      return NoBytes;
    case llvm::ELF::R_ARM_ABS32:
    case llvm::ELF::R_ARM_REL32:
    case llvm::ELF::R_ARM_THM_CALL:
    case llvm::ELF::R_ARM_CALL:
    case llvm::ELF::R_ARM_JUMP24:
    case llvm::ELF::R_ARM_PREL31:
      return WordBytes;
    default:
      return std::nullopt;
    }
  case Target::AArch64:
    switch (Type) {
    case llvm::ELF::R_AARCH64_ABS64:
    case llvm::ELF::R_AARCH64_PREL64:
      return DoubleWordBytes;
    case llvm::ELF::R_AARCH64_PREL32:
    case llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21:
    case llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_JUMP26:
    case llvm::ELF::R_AARCH64_CALL26:
    case llvm::ELF::R_AARCH64_LDST16_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC:
    case llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC:
      return WordBytes;
    default:
      return std::nullopt;
    }
  case Target::RISCV64:
    switch (Type) {
    case llvm::ELF::R_RISCV_64:
    case llvm::ELF::R_RISCV_CALL:
    case llvm::ELF::R_RISCV_CALL_PLT:
      return DoubleWordBytes;
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
    case llvm::ELF::R_RISCV_PCREL_LO12_S:
    case llvm::ELF::R_RISCV_32_PCREL:
    case llvm::ELF::R_RISCV_ADD32:
    case llvm::ELF::R_RISCV_SUB32:
      return WordBytes;
    case llvm::ELF::R_RISCV_SET6:
    case llvm::ELF::R_RISCV_SUB6:
      return ByteBytes;
    case llvm::ELF::R_RISCV_RELAX:
      return NoBytes;
    default:
      return std::nullopt;
    }
  case Target::S390X:
    switch (Type) {
    case llvm::ELF::R_390_64:
      return DoubleWordBytes;
    case llvm::ELF::R_390_PC32:
    case llvm::ELF::R_390_PC32DBL:
    case llvm::ELF::R_390_PLT32DBL:
      return WordBytes;
    default:
      return std::nullopt;
    }
  default:
    return std::nullopt;
  }
}

void append32(std::vector<WasmEdge::Byte> &Bytes, uint32_t Value,
              Endianness Endian = Endianness::Little) {
  for (size_t I = 0; I < 4; ++I) {
    const size_t Shift = Endian == Endianness::Little ? I * 8 : (3 - I) * 8;
    Bytes.push_back(static_cast<WasmEdge::Byte>(Value >> Shift));
  }
}

std::vector<WasmEdge::Byte>
makeMachOBuildVersionCommand(uint32_t Platform, uint32_t MinimumOS,
                             uint32_t SDK,
                             std::initializer_list<MachOBuildToolVersion> Tools,
                             Endianness Endian = Endianness::Little) {
  std::vector<WasmEdge::Byte> Bytes;
  append32(Bytes, llvm::MachO::LC_BUILD_VERSION, Endian);
  append32(Bytes, 24 + static_cast<uint32_t>(Tools.size()) * 8, Endian);
  append32(Bytes, Platform, Endian);
  append32(Bytes, MinimumOS, Endian);
  append32(Bytes, SDK, Endian);
  append32(Bytes, static_cast<uint32_t>(Tools.size()), Endian);
  for (const auto &Tool : Tools) {
    append32(Bytes, Tool.Tool, Endian);
    append32(Bytes, Tool.Version, Endian);
  }
  return Bytes;
}

std::vector<WasmEdge::Byte> makeLoadCommandOnlyMachO(
    std::initializer_list<std::vector<WasmEdge::Byte>> Commands) {
  uint32_t CommandBytes = 0;
  for (const auto &Command : Commands)
    CommandBytes += static_cast<uint32_t>(Command.size());
  std::vector<WasmEdge::Byte> Bytes;
  append32(Bytes, llvm::MachO::MH_MAGIC_64);
  append32(Bytes, llvm::MachO::CPU_TYPE_X86_64);
  append32(Bytes, llvm::MachO::CPU_SUBTYPE_X86_64_ALL);
  append32(Bytes, llvm::MachO::MH_OBJECT);
  append32(Bytes, static_cast<uint32_t>(Commands.size()));
  append32(Bytes, CommandBytes);
  append32(Bytes, 0);
  append32(Bytes, 0);
  for (const auto &Command : Commands)
    Bytes.insert(Bytes.end(), Command.begin(), Command.end());
  return Bytes;
}

void clearMachOSectionInstructionAttributes(std::vector<WasmEdge::Byte> &Bytes,
                                            std::string_view Name) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  constexpr size_t SectionFlagsOffset = 64;
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  bool Cleared = false;
  for (uint32_t I = 0; I < CommandCount; ++I) {
    const uint32_t Command = read32le(Bytes, CommandOffset);
    const uint32_t CommandSize = read32le(Bytes, CommandOffset + 4);
    if (Command == llvm::MachO::LC_SEGMENT_64) {
      const uint32_t SectionCount = read32le(Bytes, CommandOffset + 64);
      for (uint32_t J = 0; J < SectionCount; ++J) {
        const size_t SectionOffset =
            CommandOffset + SegmentCommandSize + J * SectionSize;
        const std::string_view SectionName(
            reinterpret_cast<const char *>(Bytes.data() + SectionOffset), 16);
        if (SectionName.substr(0, SectionName.find('\0')) != Name)
          continue;
        const uint32_t Flags =
            read32le(Bytes, SectionOffset + SectionFlagsOffset);
        write32le(Bytes, SectionOffset + SectionFlagsOffset,
                  Flags & ~(llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                            llvm::MachO::S_ATTR_SOME_INSTRUCTIONS));
        Cleared = true;
      }
    }
    CommandOffset += CommandSize;
  }
  ASSERT_TRUE(Cleared);
}

void renameMachOSection(std::vector<WasmEdge::Byte> &Bytes,
                        std::string_view OldName, std::string_view NewName) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  ASSERT_LE(NewName.size(), 16U);
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  bool Renamed = false;
  for (uint32_t I = 0; I < CommandCount; ++I) {
    const uint32_t Command = read32le(Bytes, CommandOffset);
    const uint32_t CommandSize = read32le(Bytes, CommandOffset + 4);
    if (Command == llvm::MachO::LC_SEGMENT_64) {
      const uint32_t SectionCount = read32le(Bytes, CommandOffset + 64);
      for (uint32_t J = 0; J < SectionCount; ++J) {
        const size_t SectionOffset =
            CommandOffset + SegmentCommandSize + J * SectionSize;
        const std::string_view SectionName(
            reinterpret_cast<const char *>(Bytes.data() + SectionOffset), 16);
        if (SectionName.substr(0, SectionName.find('\0')) != OldName)
          continue;
        std::fill_n(Bytes.begin() + static_cast<ptrdiff_t>(SectionOffset), 16,
                    WasmEdge::Byte{0});
        std::copy(NewName.begin(), NewName.end(),
                  Bytes.begin() + static_cast<ptrdiff_t>(SectionOffset));
        Renamed = true;
      }
    }
    CommandOffset += CommandSize;
  }
  ASSERT_TRUE(Renamed);
}

size_t machOSectionHeaderOffset(const std::vector<WasmEdge::Byte> &Bytes,
                                std::string_view Name) {
  constexpr size_t MachHeaderSize = 32;
  constexpr size_t SegmentCommandSize = 72;
  constexpr size_t SectionSize = 80;
  const uint32_t CommandCount = read32le(Bytes, 16);
  size_t CommandOffset = MachHeaderSize;
  for (uint32_t I = 0; I < CommandCount; ++I) {
    const uint32_t Command = read32le(Bytes, CommandOffset);
    const uint32_t CommandSize = read32le(Bytes, CommandOffset + 4);
    if (Command == llvm::MachO::LC_SEGMENT_64) {
      const uint32_t SectionCount = read32le(Bytes, CommandOffset + 64);
      for (uint32_t J = 0; J < SectionCount; ++J) {
        const size_t SectionOffset =
            CommandOffset + SegmentCommandSize + J * SectionSize;
        const std::string_view SectionName(
            reinterpret_cast<const char *>(Bytes.data() + SectionOffset), 16);
        if (SectionName.substr(0, SectionName.find('\0')) == Name)
          return SectionOffset;
      }
    }
    CommandOffset += CommandSize;
  }
  ADD_FAILURE() << "generated object has no " << Name << " section";
  return 0;
}

struct X86_64MachOSectionRelocation {
  size_t Field = 0;
  size_t Relocation = 0;
  int64_t Raw = 0;
  uint64_t TargetOffset = 0;
  uint64_t TargetSize = 0;
  uint32_t Type = 0;
  bool SymbolLess = false;
  bool SectionTarget = false;
};

std::vector<WasmEdge::Byte> makeX86_64MachOSectionOffsetObject() {
  constexpr std::string_view Assembly = R"(
.text
cmpq %rax, Ldata(%rip)
ret
.data
.space 13
Ldata:
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("x86_64-apple-macosx"),
                            std::string(Assembly), {}, false);
}

X86_64MachOSectionRelocation
x86_64MachOSectionRelocation(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-relocations.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {};
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return {};
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__text")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {};
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getSymbol() != MachO->symbol_end())
        continue;
      const auto Target =
          MachO->getRelocationSection(Relocation.getRawDataRefImpl());
      if (Target == MachO->section_end())
        continue;
      const uint32_t Type = static_cast<uint32_t>(Relocation.getType());
      const int64_t Suffix = Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                          : 0;
      const auto ContentOffset =
          reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
          Bytes.data();
      EXPECT_GE(ContentOffset, 0);
      const size_t Field = static_cast<size_t>(ContentOffset) +
                           static_cast<size_t>(Relocation.getOffset());
      const int64_t Raw = static_cast<int32_t>(read32le(Bytes, Field));
      const int64_t TargetOffset =
          Raw + static_cast<int64_t>(Section.getAddress()) +
          static_cast<int64_t>(Relocation.getOffset()) + 4 + Suffix -
          static_cast<int64_t>(Target->getAddress());
      EXPECT_GE(TargetOffset, 0);
      return {Field,
              Header.reloff + Relocation.getRawDataRefImpl().d.b * 8,
              Raw,
              static_cast<uint64_t>(TargetOffset),
              Target->getSize(),
              Type,
              true,
              true};
    }
  }
  ADD_FAILURE() << "generated object has no x86_64 section relocation";
  return {};
}

struct CompactUnwindObjectOffsets {
  size_t Content;
  uint32_t Count;
};

CompactUnwindObjectOffsets
compactUnwindObjectOffsets(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {};
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return {};
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__compact_unwind")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {};
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    const auto Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    return {Content, Header.nreloc};
  }
  ADD_FAILURE() << "generated object has no compact unwind section";
  return {};
}

size_t compactUnwindRelocationFileOffset(std::vector<WasmEdge::Byte> &Bytes,
                                         uint64_t FieldOffset) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return 0;
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return 0;
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__compact_unwind")
      continue;
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() == FieldOffset)
        return Header.reloff + Relocation.getRawDataRefImpl().d.b * 8;
    }
  }
  ADD_FAILURE() << "no compact unwind relocation at " << FieldOffset;
  return 0;
}

std::vector<uint32_t>
ehFrameFDEOffsets(const std::vector<WasmEdge::Byte> &Bytes) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "compact-unwind.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {};
  }
  std::vector<uint32_t> Result;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() >= 8)
        Result.push_back(static_cast<uint32_t>(Relocation.getOffset() - 8));
    }
  }
  std::sort(Result.begin(), Result.end());
  Result.erase(std::unique(Result.begin(), Result.end()), Result.end());
  return Result;
}

size_t elf64SymbolTableSectionHeader(const std::vector<WasmEdge::Byte> &Bytes) {
  const auto Table = static_cast<size_t>(read64le(Bytes, 40));
  const auto Count = static_cast<size_t>(read32le(Bytes, 60) & 0xFFFF);
  for (size_t Index = 1; Index < Count; ++Index) {
    const auto Header = Table + Index * 64;
    if (read32le(Bytes, Header + 4) == llvm::ELF::SHT_SYMTAB)
      return Header;
  }
  return 0;
}

#if LLVM_VERSION_MAJOR < 19
#else
std::vector<WasmEdge::Byte> makeX86_64CrelObject() {
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto Offset = read64le(Bytes, Header + 24);
  const auto Info = read64le(Bytes, Offset + 8);
  const auto Addend = static_cast<int64_t>(read64le(Bytes, Offset + 16));
  const std::array<llvm::ELF::Elf_Crel<true>, 1> Relocations{{
      {read64le(Bytes, Offset), static_cast<uint32_t>(Info >> 32),
       static_cast<uint32_t>(Info), Addend},
  }};
  llvm::SmallVector<char, 16> Encoded;
  llvm::raw_svector_ostream Stream(Encoded);
  llvm::ELF::encodeCrel<true>(
      Stream, Relocations, [](const auto &Relocation) { return Relocation; });
  std::copy(Encoded.begin(), Encoded.end(), Bytes.data() + Offset);
  Bytes[Header + 4] = 0x14;
  Bytes[Header + 5] = 0x00;
  Bytes[Header + 6] = 0x00;
  Bytes[Header + 7] = 0x40;
  write64le(Bytes, Header + 32, Encoded.size());
  write64le(Bytes, Header + 56, 1);
  return Bytes;
}
#endif

std::vector<WasmEdge::Byte>
makeARMAttributeObject(std::optional<unsigned> VFPArgs,
                       std::string ExtraAssembly = {},
                       std::string Features = {}) {
  std::string Assembly = ".syntax unified\n.text\n.globl f0\nf0:\n bx lr\n";
  if (VFPArgs)
    Assembly += ".eabi_attribute 28, " + std::to_string(*VFPArgs) + "\n";
  Assembly += std::move(ExtraAssembly);
  return makeAssemblyObject(llvm::Triple("armv7-unknown-linux-gnueabi"),
                            std::move(Assembly), std::move(Features));
}

std::vector<std::pair<size_t, size_t>>
armAttributeSections(const std::vector<WasmEdge::Byte> &Bytes) {
  std::vector<std::pair<size_t, size_t>> Result;
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm-attributes.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return Result;
  }
  for (const auto &Section : (*Object)->sections()) {
    const llvm::object::ELFSectionRef ELFSection(Section);
    if (ELFSection.getType() == llvm::ELF::SHT_ARM_ATTRIBUTES)
      Result.emplace_back(static_cast<size_t>(ELFSection.getOffset()),
                          static_cast<size_t>(Section.getSize()));
  }
  return Result;
}

void markARMAttributeSection(std::vector<WasmEdge::Byte> &Bytes,
                             llvm::StringRef SectionName, uint32_t Flags = 0) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm-attributes.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return;
  }
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (Name && *Name == SectionName) {
      const size_t Header = read32le(Bytes, 32) + Section.getIndex() * 40;
      write32le(Bytes, Header + 4, llvm::ELF::SHT_ARM_ATTRIBUTES);
      write32le(Bytes, Header + 8, Flags);
      return;
    }
  }
  FAIL() << "missing section " << SectionName.str();
}

struct AArch64MachOAddendFixture {
  std::vector<WasmEdge::Byte> Bytes;
  size_t Content = 0;
  size_t RelocationCount = 0;
  std::vector<size_t> Relocations;
};

AArch64MachOAddendFixture makeAArch64MachOAddendFixture() {
  auto Bytes = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.text
.globl _entry
.globl _target
.globl _data
_entry:
  bl _target+4
  adrp x0, _data@PAGE+4097
  add x0, x0, _data@PAGEOFF+4097
  ret
.section __TEXT,__target,regular,pure_instructions
_target:
  ret
.section __DATA,__data
_data:
  .quad 0
.subsections_via_symbols
)");
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm64-addend.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return {std::move(Bytes), 0, 0, {}};
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return {std::move(Bytes), 0, 0, {}};
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__text")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    if (!Contents)
      return {std::move(Bytes), 0, 0, {}};
    AArch64MachOAddendFixture Result;
    Result.Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    Result.RelocationCount = machOSectionHeaderOffset(Bytes, "__text") + 60;
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      Result.Relocations.push_back(Header.reloff +
                                   Relocation.getRawDataRefImpl().d.b * 8);
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_ADDEND ||
          Relocation.getOffset() != 0)
        continue;
      const size_t Record = Result.Relocations.back();
      const uint32_t Word = read32le(Bytes, Record + 4);
      write32le(Bytes, Record + 4,
                (Word & UINT32_C(0xFF000000)) | UINT32_C(0x00FFFFFC));
    }
    Result.Bytes = std::move(Bytes);
    return Result;
  }
  ADD_FAILURE() << "generated object has no text section";
  return {std::move(Bytes), 0, 0, {}};
}

std::vector<WasmEdge::Byte> makeAliasedAArch64MachODwarfObject(
    std::string_view FunctionSection = "__text") {
  const std::string Assembly = ".section __TEXT," +
                               std::string(FunctionSection) +
                               R"(,regular,pure_instructions
.globl _compact_alias
.globl _fde_alias
_compact_alias:
_fde_alias:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Laliased_cie:
.long Laliased_cie_end-Laliased_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Laliased_cie_end:
Laliased_fde:
.long Laliased_fde_end-Laliased_fde-4
.long Laliased_fde+4-Laliased_cie
.quad _fde_alias-.
.quad 4
.byte 0
.p2align 3
Laliased_fde_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _compact_alias
.long 4
.long 0x03000018
.quad 0
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), Assembly);
}

std::vector<WasmEdge::Byte> makeRegularMachOFunctionSectionObject() {
  auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, {}, false, false, false, false, true, false, true);
  renameMachOSection(Bytes, "__unused_text", "__custom_text");
  clearMachOSectionInstructionAttributes(Bytes, "__custom_text");
  return Bytes;
}

std::vector<WasmEdge::Byte> makeSectionTargetAArch64MachODwarfObject() {
  constexpr std::string_view Assembly = R"(
.text
nop
.globl _section_target
_section_target:
Lsection_target:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lsection_cie:
.long Lsection_cie_end-Lsection_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Lsection_cie_end:
Lsection_fde:
.long Lsection_fde_end-Lsection_fde-4
.long Lsection_fde+4-Lsection_cie
.quad Lsection_target-.
.quad 4
.byte 0
.p2align 3
Lsection_fde_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _section_target
.long 4
.long 0x03000018
.quad 0
.quad 0
)";
  auto Bytes = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"),
                                  std::string(Assembly));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-target.o"));
  EXPECT_TRUE(static_cast<bool>(Object));
  if (!Object) {
    llvm::consumeError(Object.takeError());
    return Bytes;
  }
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  EXPECT_NE(MachO, nullptr);
  if (MachO == nullptr)
    return Bytes;
  uint32_t TextOrdinal = 0;
  uint32_t Ordinal = 0;
  for (const auto &Section : MachO->sections()) {
    ++Ordinal;
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (Name && *Name == "__text")
      TextOrdinal = Ordinal;
  }
  EXPECT_NE(TextOrdinal, 0U);
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    EXPECT_TRUE(static_cast<bool>(Name));
    if (!Name || *Name != "__eh_frame")
      continue;
    auto Contents = Section.getContents();
    EXPECT_TRUE(static_cast<bool>(Contents));
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const size_t FileOffset =
          Header.reloff + Relocation.getRawDataRefImpl().d.b * 8 + 4;
      const uint32_t Word = read32le(Bytes, FileOffset);
      write32le(Bytes, FileOffset, (Word & UINT32_C(0xF7000000)) | TextOrdinal);
      const size_t Content = static_cast<size_t>(
          reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
          Bytes.data());
      write64le(Bytes, Content + Relocation.getOffset(),
                read64le(Bytes, Content + Relocation.getOffset()) + 4);
    }
  }
  return Bytes;
}

void setMachOEHFrameInitialLocation(std::vector<WasmEdge::Byte> &Bytes,
                                    uint64_t Value) {
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "malformed-section-target.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    auto Contents = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents));
    const size_t Content = static_cast<size_t>(
        reinterpret_cast<const WasmEdge::Byte *>(Contents->data()) -
        Bytes.data());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() == llvm::MachO::ARM64_RELOC_UNSIGNED)
        write64le(Bytes, Content + Relocation.getOffset(), Value);
    }
    return;
  }
  FAIL() << "generated object has no EH frame section";
}

std::vector<WasmEdge::Byte>
makeMultipleFDEAArch64MachODwarfObject(uint32_t Encoding) {
  std::string Assembly = R"(
.text
.globl _multiple_fde
_multiple_fde:
ret
.section __TEXT,__eh_frame,coalesced,no_toc+strip_static_syms+live_support
.p2align 3
Lmultiple_cie:
.long Lmultiple_cie_end-Lmultiple_cie-4
.long 0
.byte 1
.asciz "zR"
.byte 4
.byte 0x78
.byte 30
.byte 1
.byte 0x10
.p2align 3
Lmultiple_cie_end:
Lmultiple_fde0:
.long Lmultiple_fde0_end-Lmultiple_fde0-4
.long Lmultiple_fde0+4-Lmultiple_cie
.quad _multiple_fde-.
.quad 4
.byte 0
.p2align 3
Lmultiple_fde0_end:
Lmultiple_fde1:
.long Lmultiple_fde1_end-Lmultiple_fde1-4
.long Lmultiple_fde1+4-Lmultiple_cie
.quad _multiple_fde-.
.quad 4
.byte 0
.p2align 3
Lmultiple_fde1_end:
.long 0
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _multiple_fde
.long 4
.long )";
  Assembly += std::to_string(Encoding);
  Assembly += R"(
.quad 0
.quad 0
)";
  return makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), Assembly);
}

class UnsupportedObjectArchitectureTest
    : public LinkerOutputTest,
      public testing::WithParamInterface<ObjectArchitectureCase> {};

#if defined(__x86_64__) || defined(_M_X64)
class NonHostObjectFormatTest
    : public LinkerOutputTest,
      public testing::WithParamInterface<ObjectArchitectureCase> {};
#endif

#if WASMEDGE_OS_LINUX && (defined(__x86_64__) || defined(_M_X64))
TEST_P(UnsupportedObjectArchitectureTest,
       NativeLinkerRejectsUnsupportedInputsAtomically) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Output = Directory / "existing.wasm";
  const std::array<WasmEdge::Byte, 3> Sentinel{1, 2, 3};
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto Object = makeSemanticObject(llvm::Triple(Test.Triple));
  {
    std::ofstream File(Output, std::ios_base::binary | std::ios_base::trunc);
    File.write(reinterpret_cast<const char *>(Sentinel.data()),
               Sentinel.size());
    ASSERT_TRUE(File);
  }
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_EQ(readFile(Output),
            (std::vector<WasmEdge::Byte>(Sentinel.begin(), Sentinel.end())));
  expectNoTemporaryFiles();
}

INSTANTIATE_TEST_SUITE_P(
    NonHostArchitectures, UnsupportedObjectArchitectureTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64},
        ObjectArchitectureCase{"aarch64-unknown-linux-gnu", Target::AArch64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      switch (Info.index) {
      case 0:
        return std::string("MachO_X86_64");
      case 1:
        return std::string("COFF_X86_64");
      default:
        return architectureName(Info.param.Architecture);
      }
    });
#endif

#if defined(__x86_64__) || defined(_M_X64)
TEST_P(NonHostObjectFormatTest,
       NativeLinkerRejectsNonHostObjectFormatsAtomically) {
  constexpr std::array<WasmEdge::Byte, 8> EmptyWasm{0x00, 0x61, 0x73, 0x6D,
                                                    0x01, 0x00, 0x00, 0x00};
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Object = makeObject(llvm::Triple(Test.Triple), false, false, "f0",
                                 {}, false, false, "generic", {}, false, false,
                                 false, false, false, false, {}, true);
  const auto Output = Directory / (std::string(Test.Triple) + ".wasm");
  EXPECT_FALSE(
      NativeLinker::link(Object, EmptyWasm, Output, OutputKind::UniversalWasm));
  EXPECT_FALSE(std::filesystem::exists(Output));
  expectNoTemporaryFiles();
}

#if WASMEDGE_OS_LINUX
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "MachO_X86_64" : "COFF_X86_64";
    });
#elif WASMEDGE_OS_MACOS
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-unknown-linux-gnu", Target::X86_64},
        ObjectArchitectureCase{"x86_64-pc-windows-msvc", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "ELF_X86_64" : "COFF_X86_64";
    });
#elif WASMEDGE_OS_WINDOWS
INSTANTIATE_TEST_SUITE_P(
    NonHostFormats, NonHostObjectFormatTest,
    testing::Values(
        ObjectArchitectureCase{"x86_64-unknown-linux-gnu", Target::X86_64},
        ObjectArchitectureCase{"x86_64-apple-macosx", Target::X86_64}),
    [](const testing::TestParamInfo<ObjectArchitectureCase> &Info) {
      return Info.index == 0 ? "ELF_X86_64" : "MachO_X86_64";
    });
#endif
#endif

TEST(ObjectReaderTest, ParsesCompleteMachOBuildVersionCommand) {
  const auto Bytes = makeMachOBuildVersionCommand(
      7, UINT32_C(0x01020304), UINT32_C(0xA0B0C0D0),
      {{3, UINT32_C(0x11223344)}, {9, UINT32_C(0xFFEEDDCC)}});

  auto Parsed = Internal::parseMachOBuildVersion(Bytes, Endianness::Little);
  ASSERT_TRUE(Parsed);
  EXPECT_EQ(Parsed->Platform, 7U);
  EXPECT_EQ(Parsed->MinimumOS, UINT32_C(0x01020304));
  EXPECT_EQ(Parsed->SDK, UINT32_C(0xA0B0C0D0));
  ASSERT_EQ(Parsed->Tools.size(), 2U);
  EXPECT_EQ(Parsed->Tools[0].Tool, 3U);
  EXPECT_EQ(Parsed->Tools[0].Version, UINT32_C(0x11223344));
  EXPECT_EQ(Parsed->Tools[1].Tool, 9U);
  EXPECT_EQ(Parsed->Tools[1].Version, UINT32_C(0xFFEEDDCC));
}

TEST(ObjectReaderTest, ParsesBigEndianMachOBuildVersionCommand) {
  const auto Bytes = makeMachOBuildVersionCommand(
      UINT32_C(0x10203040), UINT32_C(0x50607080), UINT32_C(0x90A0B0C0),
      {{UINT32_C(0xD0E0F001), UINT32_C(0x12345678)}}, Endianness::Big);

  auto Parsed = Internal::parseMachOBuildVersion(Bytes, Endianness::Big);
  ASSERT_TRUE(Parsed);
  EXPECT_EQ(Parsed->Platform, UINT32_C(0x10203040));
  EXPECT_EQ(Parsed->MinimumOS, UINT32_C(0x50607080));
  EXPECT_EQ(Parsed->SDK, UINT32_C(0x90A0B0C0));
  ASSERT_EQ(Parsed->Tools.size(), 1U);
  EXPECT_EQ(Parsed->Tools[0].Tool, UINT32_C(0xD0E0F001));
  EXPECT_EQ(Parsed->Tools[0].Version, UINT32_C(0x12345678));
}

TEST(ObjectReaderTest, RejectsMalformedMachOBuildVersionCommands) {
  const auto Valid = makeMachOBuildVersionCommand(1, 2, 3, {{4, 5}});
  std::vector<std::vector<WasmEdge::Byte>> Invalid;
  Invalid.emplace_back(Valid.begin(), Valid.begin() + 23);
  Invalid.push_back(Valid);
  Invalid.back().push_back(0);
  Invalid.push_back(Valid);
  write32le(Invalid.back(), 4, 24);
  Invalid.push_back(Valid);
  write32le(Invalid.back(), 20, 2);
  Invalid.push_back(Valid);
  write32le(Invalid.back(), 20, UINT32_MAX);
  Invalid.push_back(Valid);
  write32le(Invalid.back(), 0, llvm::MachO::LC_UUID);

  for (const auto &Bytes : Invalid) {
    auto Parsed = Internal::parseMachOBuildVersion(Bytes, Endianness::Little);
    EXPECT_FALSE(Parsed);
    if (!Parsed) {
      EXPECT_EQ(Parsed.error().Kind, DiagnosticKind::Malformed);
    }
  }
}

TEST(ObjectReaderTest, PreservesRepeatedMachOBuildVersionsInCommandOrder) {
  const auto First = makeMachOBuildVersionCommand(
      1, UINT32_C(0x000D0000), UINT32_C(0x000E0000), {{3, 10}});
  const auto Second = makeMachOBuildVersionCommand(
      1, UINT32_C(0x000F0100), UINT32_C(0x00100000), {{1, 20}, {4, 30}});
  const auto Bytes = makeLoadCommandOnlyMachO({First, Second});

  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph) << (Graph ? "" : Graph.error().Message);
  ASSERT_EQ(Graph->machOBuildVersions().size(), 2U);
  EXPECT_EQ(Graph->machOBuildVersions()[0].MinimumOS, UINT32_C(0x000D0000));
  EXPECT_EQ(Graph->machOBuildVersions()[0].Tools[0].Version, 10U);
  EXPECT_EQ(Graph->machOBuildVersions()[1].MinimumOS, UINT32_C(0x000F0100));
  ASSERT_EQ(Graph->machOBuildVersions()[1].Tools.size(), 2U);
  EXPECT_EQ(Graph->machOBuildVersions()[1].Tools[0].Version, 20U);
  EXPECT_EQ(Graph->machOBuildVersions()[1].Tools[1].Version, 30U);
}

TEST(ObjectReaderTest, LeavesMachOBuildVersionsEmptyWhenCommandIsAbsent) {
  const auto Bytes = makeLoadCommandOnlyMachO({});

  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph) << (Graph ? "" : Graph.error().Message);
  EXPECT_TRUE(Graph->machOBuildVersions().empty());
}

TEST(ObjectReaderTest, ExtractsBuildVersionFromLLVMGeneratedMacOSObject) {
  for (const auto &[Triple, MinimumOS] :
       {std::pair("x86_64-apple-macosx10.15.0", UINT32_C(0x000A0F00)),
        std::pair("x86_64-apple-macosx10.13.0", UINT32_C(0x000A0D00))}) {
    REQUIRE_LLVM_TARGET(Triple);
    const auto Bytes = makeObject(llvm::Triple(Triple));

    auto Graph = ObjectReader::read(Bytes, Target::X86_64);
    ASSERT_TRUE(Graph) << (Graph ? "" : Graph.error().Message);
    ASSERT_EQ(Graph->machOBuildVersions().size(), 1U) << Triple;
    EXPECT_EQ(Graph->machOBuildVersions()[0].Platform, 1U) << Triple;
    EXPECT_EQ(Graph->machOBuildVersions()[0].MinimumOS, MinimumOS) << Triple;
  }
}

TEST(ObjectReaderTest, PropagatesMalformedMachOLoadCommandDiagnostics) {
  auto Bytes =
      makeLoadCommandOnlyMachO({makeMachOBuildVersionCommand(1, 2, 3, {})});
  write32le(Bytes, 32 + 4, 32);

  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_FALSE(Graph);
  EXPECT_EQ(Graph.error().Kind, DiagnosticKind::Malformed);
  EXPECT_NE(Graph.error().Message.find("object file parse error"),
            std::string::npos);
}

TEST(ByteIOTest, FloorDividesNegativeValues) {
  EXPECT_EQ(Internal::floorDivide(0, 4096), 0);
  EXPECT_EQ(Internal::floorDivide(4095, 4096), 0);
  EXPECT_EQ(Internal::floorDivide(4096, 4096), 1);
  EXPECT_EQ(Internal::floorDivide(-1, 4096), -1);
  EXPECT_EQ(Internal::floorDivide(-4096, 4096), -1);
  EXPECT_EQ(Internal::floorDivide(-4097, 4096), -2);
  EXPECT_EQ(Internal::floorDivide(-6, 4), -2);
  EXPECT_EQ(Internal::floorDivide(INT64_MIN, 4096), INT64_MIN / 4096);
}

TEST(ByteIOTest, SignExtendsEveryWidth) {
  EXPECT_EQ(Internal::signExtend(0x7F, 8), 127);
  EXPECT_EQ(Internal::signExtend(0x80, 8), -128);
  EXPECT_EQ(Internal::signExtend(0xFFFF, 16), -1);
  EXPECT_EQ(Internal::signExtend(UINT64_C(0x1FFFF), 16), -1);
  EXPECT_EQ(Internal::signExtend(UINT64_C(0x40000000), 31),
            -INT64_C(0x40000000));
  EXPECT_EQ(Internal::signExtend(UINT64_C(0x7FFFFFFFFFFFFFFF), 64), INT64_MAX);
  EXPECT_EQ(Internal::signExtend(UINT64_C(0x8000000000000000), 64), INT64_MIN);
  EXPECT_EQ(Internal::signExtend(~UINT64_C(0), 64), -1);
}

class GeneratedLinuxObjectTest
    : public testing::Test,
      public testing::WithParamInterface<LinuxRelocationCase> {};

TEST_P(GeneratedLinuxObjectTest, ReadsAndRelocatesGeneratedLinuxObjects) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
#if LLVM_VERSION_MAJOR < 15
  const bool SymbolDifference = false;
#else
  const bool SymbolDifference = Test.Architecture == Target::RISCV64;
#endif
  std::set<uint64_t> GeneratedTypes;
  for (const bool Optimize : {false, true}) {
    for (const bool Tuned : {false, true}) {
      for (const bool Interruptible : {false, true}) {
        const bool Exceptions = Test.Architecture != Target::ARM;
        const auto ObjectBytes = makeObject(
            llvm::Triple(Test.Triple), false, false, "representative", {}, true,
            true, Tuned ? Test.TunedCPU : "generic", Test.Features, false,
            Optimize, Interruptible, true, true, Exceptions,
            SymbolDifference
                ? ".pushsection .rodata.symbol_begin,\"a\",@progbits\n"
                  "inventory_begin:\n.byte 0\n.popsection\n"
                  ".pushsection .rodata.symbol_end,\"a\",@progbits\n"
                  "inventory_end:\n.byte 0\n.popsection\n"
                  ".pushsection .rodata.symbol_difference,\"a\",@progbits\n"
                  ".word inventory_end - inventory_begin\n"
                  ".popsection\n"
                : "");
        auto Object =
            llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
                llvm::StringRef(
                    reinterpret_cast<const char *>(ObjectBytes.data()),
                    ObjectBytes.size()),
                "inventory.o"));
        ASSERT_TRUE(static_cast<bool>(Object));
        for (const auto &Section : (*Object)->sections()) {
          for (const auto &Relocation : Section.relocations()) {
            ASSERT_LE(Relocation.getType(), UINT32_MAX);
            GeneratedTypes.insert(Relocation.getType());
            EXPECT_TRUE(Test.SupportedTypes.count(Relocation.getType()) != 0)
                << Test.Triple << " unexpected relocation "
                << Relocation.getType();
            ASSERT_TRUE(expectedELFPatchWidth(
                Test.Architecture, static_cast<uint32_t>(Relocation.getType())))
                << Test.Triple << " relocation " << Relocation.getType();
          }
        }
        auto Graph = ObjectReader::read(ObjectBytes, Test.Architecture);
        ASSERT_TRUE(Graph) << Test.Triple;
        for (const std::string_view Name :
             {"memory", "vector", "table", "direct"}) {
          EXPECT_TRUE(std::any_of(
              Graph->symbols().begin(), Graph->symbols().end(),
              [&](const auto &Value) { return Value.Name == Name; }))
              << Test.Triple << " missing representative " << Name;
        }
        if (Exceptions) {
          EXPECT_TRUE(std::any_of(Graph->sections().begin(),
                                  Graph->sections().end(),
                                  [](const auto &Value) {
                                    return Value.Kind == SectionKind::Unwind;
                                  }))
              << Test.Triple << " missing landingpad unwind data";
        }
        for (const auto &Relocation : Graph->relocations()) {
          EXPECT_EQ(expectedELFPatchWidth(Test.Architecture, Relocation.Type),
                    Relocation.PatchSize);
        }
        ASSERT_TRUE(layout(*Graph, 0x1000)) << Test.Triple;
        const auto Snapshot = *Graph;
        auto Applied = applyRelocations(*Graph);
        EXPECT_TRUE(Applied) << Test.Triple;
        if (!Applied) {
          expectGraphStateEquals(*Graph, Snapshot);
        }
      }
    }
  }
  for (const auto Type : Test.RequiredTypes) {
    if (!SymbolDifference && Test.Architecture == Target::RISCV64 &&
        (Type == llvm::ELF::R_RISCV_ADD32 || Type == llvm::ELF::R_RISCV_SUB32))
      continue;
    if (Type == llvm::ELF::R_RISCV_CALL_PLT &&
        GeneratedTypes.count(llvm::ELF::R_RISCV_CALL) != 0)
      continue;
    EXPECT_TRUE(GeneratedTypes.count(Type) != 0)
        << Test.Triple << " missing core relocation " << Type;
  }
}

INSTANTIATE_TEST_SUITE_P(
    Architectures, GeneratedLinuxObjectTest,
    testing::Values(
        LinuxRelocationCase{"armv7-unknown-linux-gnueabihf",
                            Target::ARM,
                            "cortex-a8",
                            "",
                            {llvm::ELF::R_ARM_ABS32, llvm::ELF::R_ARM_REL32,
                             llvm::ELF::R_ARM_CALL, llvm::ELF::R_ARM_PREL31},
                            {llvm::ELF::R_ARM_REL32, llvm::ELF::R_ARM_CALL,
                             llvm::ELF::R_ARM_PREL31}},
        LinuxRelocationCase{
            "aarch64-unknown-linux-gnu",
            Target::AArch64,
            "cortex-a53",
            "",
            {llvm::ELF::R_AARCH64_ABS64, llvm::ELF::R_AARCH64_PREL64,
             llvm::ELF::R_AARCH64_PREL32, llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
             llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST8_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_CALL26,
             llvm::ELF::R_AARCH64_LDST32_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST64_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_LDST128_ABS_LO12_NC},
            {llvm::ELF::R_AARCH64_ADR_PREL_PG_HI21,
             llvm::ELF::R_AARCH64_ADD_ABS_LO12_NC,
             llvm::ELF::R_AARCH64_CALL26}},
        LinuxRelocationCase{
            "riscv64-unknown-linux-gnu",
            Target::RISCV64,
            "generic-rv64",
            "+a",
            {llvm::ELF::R_RISCV_64, llvm::ELF::R_RISCV_CALL,
             llvm::ELF::R_RISCV_CALL_PLT, llvm::ELF::R_RISCV_PCREL_HI20,
             llvm::ELF::R_RISCV_PCREL_LO12_I, llvm::ELF::R_RISCV_PCREL_LO12_S,
             llvm::ELF::R_RISCV_32_PCREL, llvm::ELF::R_RISCV_ADD32,
             llvm::ELF::R_RISCV_SUB32, llvm::ELF::R_RISCV_SET6,
             llvm::ELF::R_RISCV_SUB6},
            {llvm::ELF::R_RISCV_CALL_PLT, llvm::ELF::R_RISCV_PCREL_HI20,
             llvm::ELF::R_RISCV_PCREL_LO12_I, llvm::ELF::R_RISCV_ADD32,
             llvm::ELF::R_RISCV_SUB32}},
        LinuxRelocationCase{
            "s390x-unknown-linux-gnu",
            Target::S390X,
            "z13",
            "",
            {llvm::ELF::R_390_PC32, llvm::ELF::R_390_PC32DBL,
             llvm::ELF::R_390_PLT32DBL, llvm::ELF::R_390_64},
            {llvm::ELF::R_390_PC32DBL, llvm::ELF::R_390_PLT32DBL}}),
    linuxRelocationCaseName);

TEST(ObjectReaderTest, RejectsAArch64MachOSectionPageOffRelocation) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Object = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.text
.globl _entry
_entry:
  adrp x0, Ldata@PAGE
  add x0, x0, Ldata@PAGEOFF
  adrp x1, Ldata@PAGE
  ldr x1, [x1, Ldata@PAGEOFF]
  ret
.data
Ldata:
  .space 32
)");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "pageoff.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  std::vector<size_t> PageOffs;
  uint32_t TargetOrdinal = 0;
  for (const auto &Section : MachO->sections()) {
    const auto Header = MachO->getSection64(Section.getRawDataRefImpl());
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_PAGEOFF12)
        continue;
      const auto Symbol = Relocation.getSymbol();
      ASSERT_NE(Symbol, MachO->symbol_end());
      auto TargetSection = Symbol->getSection();
      ASSERT_TRUE(static_cast<bool>(TargetSection));
      ASSERT_NE(*TargetSection, MachO->section_end());
      PageOffs.push_back(Header.reloff +
                         Relocation.getRawDataRefImpl().d.b * 8 + 4);
      TargetOrdinal = static_cast<uint32_t>((*TargetSection)->getIndex());
    }
  }
  ASSERT_EQ(PageOffs.size(), 2U);
  ASSERT_NE(TargetOrdinal, 0U);
  for (const size_t Metadata : PageOffs) {
    uint32_t Word = read32le(Object, Metadata);
    Word &= ~UINT32_C(0x08FFFFFF);
    Word |= TargetOrdinal;
    write32le(Object, Metadata, Word);
  }

  Parsed = llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
      llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                      Object.size()),
      "section-pageoff.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  size_t SectionPageOffs = 0;
  for (const auto &Section : (*Parsed)->sections())
    for (const auto &Relocation : Section.relocations())
      SectionPageOffs +=
          Relocation.getType() == llvm::MachO::ARM64_RELOC_PAGEOFF12 &&
          Relocation.getSymbol() == (*Parsed)->symbol_end();
  ASSERT_EQ(SectionPageOffs, 2U);

  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_FALSE(Graph);
  EXPECT_EQ(Graph.error().Kind, DiagnosticKind::Unsupported);
  EXPECT_NE(Graph.error().Message.find(
                "non-external AArch64 Mach-O section relocation"),
            std::string::npos)
      << Graph.error().Message;
}

TEST(ObjectReaderTest, ConsumesAArch64MachOAddendPairs) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Fixture = makeAArch64MachOAddendFixture();
  ASSERT_EQ(Fixture.Relocations.size(), 6U);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 3U);
  const std::array<std::pair<uint32_t, int64_t>, 3> Expected{{
      {llvm::MachO::ARM64_RELOC_BRANCH26, -4},
      {llvm::MachO::ARM64_RELOC_PAGE21, 4097},
      {llvm::MachO::ARM64_RELOC_PAGEOFF12, 4097},
  }};
  for (const auto &Entry : Expected) {
    const uint32_t Type = Entry.first;
    const int64_t Addend = Entry.second;
    const auto Relocation =
        std::find_if(Graph->relocations().begin(), Graph->relocations().end(),
                     [&](const auto &Value) { return Value.Type == Type; });
    ASSERT_NE(Relocation, Graph->relocations().end()) << Type;
    EXPECT_EQ(Relocation->Addend, Addend) << Type;
    EXPECT_FALSE(Relocation->AddendIsImplicit) << Type;
  }

  ASSERT_TRUE(layout(*Graph, 0x4000, 0x4000));
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Relocation : Graph->relocations()) {
    const auto &Symbol = Graph->symbols()[Relocation.Symbol];
    const uint64_t Target = Graph->sections()[Symbol.Section].Address +
                            Symbol.Offset +
                            static_cast<uint64_t>(Relocation.Addend);
    const uint64_t Patch =
        Graph->sections()[Relocation.Section].Address + Relocation.Offset;
    const auto Word =
        Internal::readUnsigned(Graph->sections()[Relocation.Section].Content,
                               Relocation.Offset, 4, Endianness::Little);
    ASSERT_TRUE(Word);
    if (Relocation.Type == llvm::MachO::ARM64_RELOC_BRANCH26) {
      const uint32_t Immediate =
          static_cast<uint32_t>(*Word) & UINT32_C(0x03FFFFFF);
      int64_t Delta = static_cast<int64_t>(Immediate);
      if ((Immediate & UINT32_C(0x02000000)) != 0)
        Delta -= INT64_C(1) << 26;
      Delta *= 4;
      EXPECT_EQ(static_cast<uint64_t>(static_cast<int64_t>(Patch) + Delta),
                Target);
    } else if (Relocation.Type == llvm::MachO::ARM64_RELOC_PAGE21) {
      EXPECT_EQ(decodeAdrpTargetPage(static_cast<uint32_t>(*Word), Patch),
                Target & ~UINT64_C(0xFFF));
    } else {
      EXPECT_EQ((static_cast<uint32_t>(*Word) >> 10) & UINT32_C(0xFFF),
                Target & UINT64_C(0xFFF));
    }
  }
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOAddendPairs) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Original = makeAArch64MachOAddendFixture();
  ASSERT_EQ(Original.Relocations.size(), 6U);
  const auto SetType = [](std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                          uint32_t Type) {
    const uint32_t Word = read32le(Bytes, Offset + 4);
    write32le(Bytes, Offset + 4, (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28));
  };
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message,
                         DiagnosticKind Kind = DiagnosticKind::Malformed) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
    EXPECT_EQ(Result.error().Kind, Kind);
  };

  auto MissingSuccessor = Original.Bytes;
  write32le(MissingSuccessor, Original.RelocationCount, 1);
  Reject(std::move(MissingSuccessor), "addend relocation lacks successor");

  auto WrongSuccessor = Original.Bytes;
  SetType(WrongSuccessor, Original.Relocations[1],
          llvm::MachO::ARM64_RELOC_UNSIGNED);
  Reject(std::move(WrongSuccessor), "addend relocation successor",
         DiagnosticKind::Unsupported);

  auto MalformedAddendMetadata = Original.Bytes;
  write32le(MalformedAddendMetadata, Original.Relocations[0] + 4,
            read32le(MalformedAddendMetadata, Original.Relocations[0] + 4) |
                UINT32_C(1) << 24);
  Reject(std::move(MalformedAddendMetadata),
         "malformed addend relocation metadata");

  auto MalformedSuccessorMetadata = Original.Bytes;
  write32le(MalformedSuccessorMetadata, Original.Relocations[1] + 4,
            read32le(MalformedSuccessorMetadata, Original.Relocations[1] + 4) &
                ~(UINT32_C(1) << 27));
  Reject(std::move(MalformedSuccessorMetadata),
         "malformed addend relocation successor");

  auto MismatchedAddress = Original.Bytes;
  write32le(MismatchedAddress, Original.Relocations[0],
            read32le(MismatchedAddress, Original.Relocations[0]) + 4);
  Reject(std::move(MismatchedAddress), "addend relocation addresses differ");

  auto EmbeddedAddend = Original.Bytes;
  write32le(EmbeddedAddend, Original.Content,
            read32le(EmbeddedAddend, Original.Content) | 1);
  Reject(std::move(EmbeddedAddend),
         "addend relocation conflicts with embedded addend");
}

TEST(ObjectReaderTest, RejectsFourByteAArch64MachOUnsigned) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Object = makeAssemblyObject(llvm::Triple("arm64-apple-macosx"), R"(
.data
.globl _pointer
_pointer:
.long _target
.long 0x44332211
.text
.globl _target
_target:
ret
)");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unsigned32.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool Found = false;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Contents = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Contents));
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const auto *MachO = llvm::cast<llvm::object::MachOObjectFile>(&**Parsed);
      const auto Raw = MachO->getRelocation(Relocation.getRawDataRefImpl());
      EXPECT_EQ(MachO->getAnyRelocationLength(Raw), 2U);
      ASSERT_GE(Contents->size(), Relocation.getOffset() + 8);
      EXPECT_EQ(static_cast<uint8_t>((*Contents)[Relocation.getOffset() + 4]),
                0x11U);
      Found = true;
    }
  }
  ASSERT_TRUE(Found);
  EXPECT_FALSE(ObjectReader::read(Object, Target::AArch64));
}

class PortableObjectTest
    : public testing::Test,
      public testing::WithParamInterface<PortableObjectCase> {};

TEST_P(PortableObjectTest, ReadsRealisticPortableObjectsWithoutPersonality) {
  const auto &Test = GetParam();
  REQUIRE_LLVM_TARGET(Test.Triple);
  const auto Bytes = makeObject(llvm::Triple(Test.Triple));
  auto Graph = ObjectReader::read(Bytes, Test.Architecture);
  ASSERT_TRUE(Graph) << Test.Triple;
  std::set<uint32_t> Types;
  for (const auto &Relocation : Graph->relocations()) {
    Types.insert(Relocation.Type);
  }
  EXPECT_EQ(Types, Test.Types) << Test.Triple;
  EXPECT_TRUE(std::none_of(Graph->sections().begin(), Graph->sections().end(),
                           [](const auto &Section) {
                             return Section.Purpose ==
                                    SectionPurpose::CompactUnwind;
                           }));
  ASSERT_TRUE(layout(*Graph, 0x4000, 0x4000));
  EXPECT_TRUE(applyRelocations(*Graph)) << Test.Triple;
}

INSTANTIATE_TEST_SUITE_P(
    MachO, PortableObjectTest,
    testing::Values(PortableObjectCase{"x86_64-apple-macosx",
                                       Target::X86_64,
                                       {llvm::MachO::X86_64_RELOC_SIGNED}},
                    PortableObjectCase{"arm64-apple-macosx",
                                       Target::AArch64,
                                       {llvm::MachO::ARM64_RELOC_PAGE21,
                                        llvm::MachO::ARM64_RELOC_PAGEOFF12}}),
    portableObjectCaseName);
#if LLVM_VERSION_MAJOR < 23
constexpr uint32_t AArch64COFFPageOffset =
    llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12A;
#else
constexpr uint32_t AArch64COFFPageOffset =
    llvm::COFF::IMAGE_REL_ARM64_PAGEOFFSET_12L;
#endif

INSTANTIATE_TEST_SUITE_P(
    COFF, PortableObjectTest,
    testing::Values(PortableObjectCase{"x86_64-pc-windows-msvc",
                                       Target::X86_64,
                                       {llvm::COFF::IMAGE_REL_AMD64_REL32}},
                    PortableObjectCase{
                        "aarch64-pc-windows-msvc",
                        Target::AArch64,
                        {llvm::COFF::IMAGE_REL_ARM64_PAGEBASE_REL21,
                         AArch64COFFPageOffset}}),
    portableObjectCaseName);

TEST(ObjectReaderTest, SelectsNativeFltusedPolicyFromCOFFObject) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto COFF =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(Marker));

  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::COFF),
            ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused);
  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::ELF),
            ObjectReaderInputPolicy::Strict);
  EXPECT_EQ(Internal::nativeObjectInputPolicy(ObjectFormat::MachO),
            ObjectReaderInputPolicy::Strict);
  EXPECT_TRUE(ObjectReader::read(
      COFF, Target::X86_64,
      Internal::nativeObjectInputPolicy(ObjectFormat::COFF)));
}

TEST(ObjectReaderTest, RejectsUnresolvedMinGWFltusedUnderDefaultPolicy) {
  REQUIRE_LLVM_TARGET("x86_64-w64-windows-gnu");
  constexpr std::string_view Marker = R"(
.def _fltused;
.scl 2;
.type 0;
.endef
)";
  const auto GNUCOFF =
      makeObject(llvm::Triple("x86_64-w64-windows-gnu"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(Marker));
  EXPECT_FALSE(ObjectReader::read(GNUCOFF, Target::X86_64));
}

TEST(ObjectReaderTest, DoesNotApplyMSVCFltusedPolicyToELF) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto ELF =
      makeObject(llvm::Triple("x86_64-unknown-linux-gnu"), false, false, "f0",
                 {}, false, false, "generic", {}, false, false, false, false,
                 false, false, ".globl _fltused\n.quad _fltused\n");
  EXPECT_FALSE(ObjectReader::read(
      ELF, Target::X86_64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(ObjectReaderTest, RejectsUnrelatedMSVCUndefinedSymbolWithFltusedPolicy) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  const auto OtherUndefined =
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), true);
  EXPECT_FALSE(ObjectReader::read(
      OtherUndefined, Target::X86_64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(ObjectReaderTest, ReadsAArch64MSVCFloatingPointObjectWithFltusedPolicy) {
  REQUIRE_LLVM_TARGET("aarch64-pc-windows-msvc");
  const auto ARM64FP =
      makeObject(llvm::Triple("aarch64-pc-windows-msvc"), false, false, "f0",
                 {}, false, false, "generic", {}, true, true, false, false,
                 false, false, {}, false, false, true);
  auto ParsedARM64 =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(ARM64FP.data()),
                          ARM64FP.size()),
          "arm64-floating.obj"));
  ASSERT_TRUE(static_cast<bool>(ParsedARM64));
  for (const auto &Symbol : (*ParsedARM64)->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    EXPECT_NE(*Name, "_fltused");
  }
  EXPECT_TRUE(ObjectReader::read(
      ARM64FP, Target::AArch64,
      ObjectReaderInputPolicy::AllowUnreferencedMSVCFltused));
}

TEST(ObjectReaderTest, ParsesPairedAArch64MachOEHFrameReference) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAArch64MachOEHFrameObject();
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  const auto &Reference = Graph->ehFrameReferences().front();
  auto Fields = machOEHFrameFields(Graph->sections()[Reference.Section].Content,
                                   Target::AArch64);
  ASSERT_TRUE(Fields);
  EXPECT_EQ(*Fields, (std::set<size_t>{static_cast<size_t>(Reference.Offset)}));
  EXPECT_EQ(Graph->sections()[Reference.Section].Purpose,
            SectionPurpose::EHFrame);
  EXPECT_EQ(Graph->symbols()[Reference.Symbol].Name, "_f0");
  EXPECT_TRUE(std::none_of(
      Graph->relocations().begin(), Graph->relocations().end(),
      [&](const auto &Value) { return Value.Section == Reference.Section; }));
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOEHFramePairs) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Original = makeAArch64MachOEHFrameObject();
  const auto Object = machOEHFrameObject(Original);
  ASSERT_EQ(Object.Relocations.size(), 2U);
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message,
                         DiagnosticKind Kind = DiagnosticKind::Malformed) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
    EXPECT_EQ(Result.error().Kind, Kind);
  };
  const auto SetType = [](std::vector<WasmEdge::Byte> &Bytes, size_t Offset,
                          uint32_t Type) {
    auto Word = read32le(Bytes, Offset + 4);
    write32le(Bytes, Offset + 4, (Word & UINT32_C(0x0FFFFFFF)) | (Type << 28));
  };

  auto OrphanSubtractor = Original;
  SetType(OrphanSubtractor, Object.Relocations[1],
          llvm::MachO::ARM64_RELOC_SUBTRACTOR);
  Reject(std::move(OrphanSubtractor), "subtractor is not followed by unsigned");

  auto OrphanUnsigned = Original;
  SetType(OrphanUnsigned, Object.Relocations[0],
          llvm::MachO::ARM64_RELOC_UNSIGNED);
  Reject(std::move(OrphanUnsigned), "unsigned relocation lacks subtractor");

  auto WrongOrder = Original;
  SetType(WrongOrder, Object.Relocations[0], llvm::MachO::ARM64_RELOC_UNSIGNED);
  SetType(WrongOrder, Object.Relocations[1],
          llvm::MachO::ARM64_RELOC_SUBTRACTOR);
  Reject(std::move(WrongOrder), "unsigned relocation lacks subtractor");

  auto MismatchedOffset = Original;
  write32le(MismatchedOffset, Object.Relocations[1], 25);
  Reject(std::move(MismatchedOffset), "pair addresses differ");

  auto UnsupportedSymbol = Original;
  auto SubtractorWord = read32le(UnsupportedSymbol, Object.Relocations[0] + 4);
  auto UnsignedWord = read32le(UnsupportedSymbol, Object.Relocations[1] + 4);
  write32le(UnsupportedSymbol, Object.Relocations[0] + 4,
            (SubtractorWord & UINT32_C(0xFF000000)) |
                (UnsignedWord & UINT32_C(0x00FFFFFF)));
  Reject(std::move(UnsupportedSymbol), "subtractor symbol",
         DiagnosticKind::Unsupported);

  auto NonFDEField = Original;
  write32le(NonFDEField, Object.Relocations[0], 8);
  write32le(NonFDEField, Object.Relocations[1], 8);
  Reject(std::move(NonFDEField), "not an FDE initial-location field");
}

TEST(ObjectReaderTest, PreservesX86MachOEHFrameRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(false);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::any_of(Graph->relocations().begin(),
                          Graph->relocations().end(), [&](const auto &Value) {
                            return Graph->sections()[Value.Section].Purpose ==
                                       SectionPurpose::EHFrame &&
                                   Value.Type ==
                                       llvm::MachO::X86_64_RELOC_UNSIGNED;
                          }));
  EXPECT_TRUE(Graph->ehFrameReferences().empty());
}

TEST(ObjectReaderTest, AssociatesRawX86_64MachOFDEReference) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_TRUE(Object.Relocations.empty());
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->ehFrameReferences().size(), 1U);
  const auto &Reference = Graph->ehFrameReferences().front();
  ASSERT_LT(Reference.Section, Graph->sections().size());
  ASSERT_LT(Reference.Symbol, Graph->symbols().size());
  EXPECT_EQ(Graph->sections()[Reference.Section].Purpose,
            SectionPurpose::EHFrame);
  auto Fields = machOEHFrameFields(Graph->sections()[Reference.Section].Content,
                                   Target::X86_64);
  ASSERT_TRUE(Fields);
  EXPECT_EQ(*Fields, (std::set<size_t>{static_cast<size_t>(Reference.Offset)}));
  EXPECT_EQ(Reference.Offset, Fixture.Field);
  EXPECT_EQ(Graph->symbols()[Reference.Symbol].Name, "_f1");
}

TEST(ObjectReaderTest, RejectsStrictRawX86_64MachOFDETargets) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Reject = [](X86_64MachOEHFrameFixture Fixture, uint64_t Value,
                         std::string_view Message) {
    const auto Object = machOEHFrameObject(Fixture.Bytes);
    ASSERT_NE(Object.Content, 0U);
    write64le(Fixture.Bytes, Object.Content + Fixture.Field, Value);
    auto Result = ObjectReader::read(Fixture.Bytes, Target::X86_64);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };

  const auto Fixture = makeX86_64MachOEHFrameFixture(true);
  Reject(Fixture, Fixture.DataAddress + 1 - Fixture.FieldAddress,
         "unresolved x86_64 Mach-O FDE target");
  Reject(Fixture, UINT64_C(0x8000000000000000),
         "x86_64 Mach-O FDE target address overflows");
  Reject(Fixture, Fixture.DataAddress - Fixture.FieldAddress,
         "x86_64 Mach-O FDE target is not a text symbol");
}

TEST(ObjectReaderTest, RejectsMalformedRawX86_64MachOEHFrame) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Fixture = makeX86_64MachOEHFrameFixture(true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  write32le(Fixture.Bytes, Object.Content, 1);
  auto Result = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "malformed x86_64 Mach-O EH frame");
}

TEST(ObjectReaderTest, SkipsRelocatedX86_64MachOFDEReference) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(false);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_EQ(Object.Relocations.size(), 1U);
  auto Graph = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  const auto Relocation =
      std::find_if(Graph->relocations().begin(), Graph->relocations().end(),
                   [&](const auto &Value) {
                     return Graph->sections()[Value.Section].Purpose ==
                                SectionPurpose::EHFrame &&
                            Value.Offset == Fixture.Field;
                   });
  ASSERT_NE(Relocation, Graph->relocations().end());
  EXPECT_TRUE(std::none_of(Graph->ehFrameReferences().begin(),
                           Graph->ehFrameReferences().end(),
                           [&](const auto &Reference) {
                             return Reference.Section == Relocation->Section &&
                                    Reference.Offset == Relocation->Offset;
                           }));
}

TEST(ObjectReaderTest, SelectsDeterministicRawX86_64MachOFDEAlias) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Fixture = makeX86_64MachOEHFrameFixture(true, true);
  const auto Object = machOEHFrameObject(Fixture.Bytes);
  ASSERT_NE(Object.Content, 0U);
  ASSERT_TRUE(Object.Relocations.empty());
  auto First = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  auto Second = ObjectReader::read(Fixture.Bytes, Target::X86_64);
  ASSERT_TRUE(First);
  ASSERT_TRUE(Second);
  const auto AssertAliasedSymbols = [](const LinkGraph &Graph) {
    const auto F1 =
        std::find_if(Graph.symbols().begin(), Graph.symbols().end(),
                     [](const auto &Symbol) { return Symbol.Name == "_f1"; });
    const auto Alias = std::find_if(
        Graph.symbols().begin(), Graph.symbols().end(),
        [](const auto &Symbol) { return Symbol.Name == "_f1_alias"; });
    ASSERT_NE(F1, Graph.symbols().end());
    ASSERT_NE(Alias, Graph.symbols().end());
    ASSERT_EQ(Graph.sections()[F1->Section].Kind, SectionKind::Text);
    ASSERT_EQ(Graph.sections()[Alias->Section].Kind, SectionKind::Text);
    EXPECT_EQ(Graph.sections()[F1->Section].InputAddress + F1->Offset,
              Graph.sections()[Alias->Section].InputAddress + Alias->Offset);
  };
  AssertAliasedSymbols(*First);
  AssertAliasedSymbols(*Second);
  ASSERT_EQ(First->ehFrameReferences().size(), 1U);
  ASSERT_EQ(Second->ehFrameReferences().size(), 1U);
  const auto &FirstReference = First->ehFrameReferences().front();
  const auto &SecondReference = Second->ehFrameReferences().front();
  ASSERT_LT(FirstReference.Symbol, First->symbols().size());
  ASSERT_LT(SecondReference.Symbol, Second->symbols().size());
  const auto &FirstSymbol = First->symbols()[FirstReference.Symbol];
  const auto &SecondSymbol = Second->symbols()[SecondReference.Symbol];
  EXPECT_EQ(FirstSymbol.Name, SecondSymbol.Name);
  EXPECT_NE(FirstSymbol.Name, "$section");
  const auto F1 =
      std::find_if(First->symbols().begin(), First->symbols().end(),
                   [](const auto &Symbol) { return Symbol.Name == "_f1"; });
  ASSERT_NE(F1, First->symbols().end());
  EXPECT_EQ(First->sections()[FirstSymbol.Section].InputAddress +
                FirstSymbol.Offset,
            First->sections()[F1->Section].InputAddress + F1->Offset);
}

TEST(ObjectReaderTest, ParsesCompactOnlyAArch64MachOUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto CompactOnly =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(CompactOnly, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  EXPECT_EQ(Graph->symbols()[Record.Function].Name, "_f0");
  EXPECT_GT(Record.Length, 0U);
  EXPECT_EQ(Record.Encoding, 0x02000000U);
  EXPECT_FALSE(Record.Personality);
  EXPECT_FALSE(Record.LSDA);
  EXPECT_FALSE(Record.FDE);
  EXPECT_TRUE(std::none_of(Graph->sections().begin(), Graph->sections().end(),
                           [](const Section &Value) {
                             return Value.Purpose ==
                                    SectionPurpose::CompactUnwind;
                           }));
}

TEST(ObjectReaderTest, ParsesCompactUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  EXPECT_FALSE(Graph->compactUnwind().empty());
}

TEST(ObjectReaderTest, ResolvesCompactUnwindTargetOffsets) {
  auto Local =
      Internal::resolveCompactUnwindTargetOffset(false, 0x1000, 0, 0x1018);
  auto ExternalNegative = Internal::resolveCompactUnwindTargetOffset(
      true, 0x1000, 0x20, UINT64_MAX - 7);
  ASSERT_TRUE(Local);
  ASSERT_TRUE(ExternalNegative);
  EXPECT_EQ(*Local, 0x18U);
  EXPECT_EQ(*ExternalNegative, 0x18U);
  EXPECT_FALSE(
      Internal::resolveCompactUnwindTargetOffset(false, 0x1000, 0, 0x0FFF));
  EXPECT_FALSE(Internal::resolveCompactUnwindTargetOffset(
      true, 0, UINT64_MAX, static_cast<uint64_t>(INT64_MAX)));
}

TEST(ObjectReaderTest, ValidatesDecodedAArch64CompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> AArch64{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Records =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, AArch64);
  ASSERT_TRUE(Records);
  ASSERT_EQ(Records->size(), 1U);
  EXPECT_EQ((*Records)[0].Length, 8U);
}

TEST(ObjectReaderTest, ValidatesDecodedX86_64CompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> X86_64{
      {{0, llvm::MachO::X86_64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Records =
      Internal::parseCompactUnwindSection(Target::X86_64, Content, X86_64);
  ASSERT_TRUE(Records);
  ASSERT_EQ(Records->size(), 1U);
  EXPECT_EQ((*Records)[0].Encoding, 0x02000000U);
}

TEST(ObjectReaderTest, RejectsDecodedARMCompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  write32le(Content, 12, 0x02000000);
  const std::array<Internal::CompactUnwindRelocation, 1> AArch64{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  EXPECT_FALSE(
      Internal::parseCompactUnwindSection(Target::ARM, Content, AArch64));
}

TEST(ObjectReaderTest, DiagnosesMalformedDecodedCompactUnwind) {
  std::vector<WasmEdge::Byte> Content(32);
  write32le(Content, 8, 8);
  const auto Diagnose = [&](Internal::CompactUnwindRelocation Relocation) {
    auto Result = Internal::parseCompactUnwindSection(
        Target::AArch64, Content,
        WasmEdge::Span<const Internal::CompactUnwindRelocation>(&Relocation,
                                                                1));
    EXPECT_FALSE(Result);
    return Result.error().Message;
  };
  EXPECT_EQ(
      Diagnose({8, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}),
      "unsupported compact unwind relocation field");
  EXPECT_EQ(
      Diagnose({0, llvm::MachO::ARM64_RELOC_UNSIGNED, 4, false, false, false}),
      "unsupported compact unwind relocation");
  auto Missing =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, {});
  ASSERT_FALSE(Missing);
  EXPECT_EQ(Missing.error().Message,
            "compact unwind record lacks function relocation");
  write64le(Content, 16, 1);
  std::array<Internal::CompactUnwindRelocation, 1> Function{
      {{0, llvm::MachO::ARM64_RELOC_UNSIGNED, 8, false, false, false}}};
  auto Unrelocated =
      Internal::parseCompactUnwindSection(Target::AArch64, Content, Function);
  ASSERT_FALSE(Unrelocated);
  EXPECT_EQ(Unrelocated.error().Message,
            "compact unwind target lacks relocation");
}

TEST(ObjectReaderTest, AssociatesZeroDwarfInputByFunctionFDE) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Mixed =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  clearMachOSectionRelocations(Mixed, "__text");
  const auto Offsets = compactUnwindObjectOffsets(Mixed);
  ASSERT_NE(Offsets.Content, 0U);
  EXPECT_EQ(read32le(Mixed, Offsets.Content + 12), UINT32_C(0x03000000));
  const auto FDEOffsets = ehFrameFDEOffsets(Mixed);
  ASSERT_FALSE(FDEOffsets.empty());
  EXPECT_GT(FDEOffsets.front(), 0U);
  auto Graph = ObjectReader::read(Mixed, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_GT(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, 0U);
}

TEST(ObjectReaderTest, AssociatesExactNonzeroDwarfOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Mixed =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  clearMachOSectionRelocations(Mixed, "__text");
  const auto Offsets = compactUnwindObjectOffsets(Mixed);
  ASSERT_NE(Offsets.Content, 0U);
  std::optional<LinkGraph> Graph;
  uint32_t FDEOffset = 0;
  for (const uint32_t Candidate : ehFrameFDEOffsets(Mixed)) {
    auto CandidateObject = Mixed;
    write32le(CandidateObject, Offsets.Content + 12,
              UINT32_C(0x03000000) | Candidate);
    auto Parsed = ObjectReader::read(CandidateObject, Target::AArch64);
    if (!Parsed)
      continue;
    ASSERT_FALSE(Graph);
    FDEOffset = Candidate;
    Graph.emplace(std::move(*Parsed));
  }
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, FDEOffset);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, AssociatesDwarfFDEThroughEquivalentMachOSymbols) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeAliasedAArch64MachODwarfObject();
  const auto Inventory = collectCompactUnwindInventory(Bytes);
  ASSERT_TRUE(Inventory.HasCompactUnwind);
  ASSERT_TRUE(Inventory.HasEHFrame);
  ASSERT_EQ(Inventory.Records.size(), 1U);
  EXPECT_EQ(Inventory.Records[0].Encoding, UINT32_C(0x03000018));

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  ASSERT_TRUE(Record.FDE);
  EXPECT_EQ(Graph->symbols()[*Record.FDE].Offset, 24U);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, AssociatesSectionTargetDwarfFDEAtFunctionOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeSectionTargetAArch64MachODwarfObject();
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-target.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  bool SectionTarget = false;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__eh_frame")
      continue;
    for (const auto &Relocation : Section.relocations()) {
      if (Relocation.getOffset() != 32 ||
          Relocation.getType() != llvm::MachO::ARM64_RELOC_UNSIGNED)
        continue;
      const auto Symbol = Relocation.getSymbol();
      SectionTarget = Symbol == (*Object)->symbol_end();
    }
  }
  ASSERT_TRUE(SectionTarget);

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  const auto &Function = Graph->symbols()[Graph->compactUnwind()[0].Function];
  EXPECT_EQ(Function.Offset, 4U);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset, 24U);
  EXPECT_TRUE(compactUnwindToEHFrame(*Graph));
}

TEST(ObjectReaderTest, RejectsOverflowingSectionTargetDwarfFDEAddend) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeSectionTargetAArch64MachODwarfObject();
  setMachOEHFrameInitialLocation(Bytes, UINT64_C(0x8000000000000000));
  auto Result = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_FALSE(Result);
  EXPECT_NE(Result.error().Message.find("DWARF FDE target address overflows"),
            std::string::npos);
}

TEST(ObjectReaderTest, SelectsEncodedDwarfFDEFromMultipleCandidates) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeMultipleFDEAArch64MachODwarfObject(UINT32_C(0x03000000));
  const auto FDEs = ehFrameFDEOffsets(Bytes);
  ASSERT_EQ(FDEs.size(), 2U);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_NE(Offsets.Content, 0U);
  write32le(Bytes, Offsets.Content + 12, UINT32_C(0x03000000) | FDEs.back());

  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  ASSERT_TRUE(Graph->compactUnwind()[0].FDE);
  EXPECT_EQ(Graph->symbols()[*Graph->compactUnwind()[0].FDE].Offset,
            FDEs.back());
}

TEST(ObjectReaderTest, RejectsAmbiguousZeroOffsetDwarfFDE) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes =
      makeMultipleFDEAArch64MachODwarfObject(UINT32_C(0x03000000));
  ASSERT_EQ(ehFrameFDEOffsets(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, ClassifiesMachOCustomFunctionSectionAsText) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Bytes = makeRegularMachOFunctionSectionObject();
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "custom-section.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  bool PriorReadOnly = false;
  for (const auto &Section : (*Object)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__custom_text")
      continue;
    const auto *MachO =
        llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
    ASSERT_NE(MachO, nullptr);
    const uint32_t Flags =
        MachO->getSection64(Section.getRawDataRefImpl()).flags;
    PriorReadOnly = !Section.isText() &&
                    (Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                              llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) == 0;
  }
  ASSERT_TRUE(PriorReadOnly);
  auto Graph = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  ASSERT_LT(Function->Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Function->Section].Name, "__custom_text");
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
  const auto DataObject =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, {}, false, false, false, false, true);
  auto DataGraph = ObjectReader::read(DataObject, Target::AArch64);
  ASSERT_TRUE(DataGraph);
  const auto Data = std::find_if(
      DataGraph->symbols().begin(), DataGraph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_data"; });
  ASSERT_NE(Data, DataGraph->symbols().end());
  EXPECT_NE(DataGraph->sections()[Data->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, ClassifiesUnusedMachOInstructionSectionAsText) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Object =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, true, "generic", {}, false, false, false, false, false,
                 false, {}, true, false, false, false, true);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unused-instruction.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  bool HasInstructionSection = false;
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__unused_text")
      continue;
    const uint32_t Flags =
        MachO->getSection64(Section.getRawDataRefImpl()).flags;
    HasInstructionSection =
        !Section.isText() &&
        (Flags & (llvm::MachO::S_ATTR_PURE_INSTRUCTIONS |
                  llvm::MachO::S_ATTR_SOME_INSTRUCTIONS)) != 0;
  }
  ASSERT_TRUE(HasInstructionSection);
  auto Graph = ObjectReader::read(Object, Target::AArch64);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_unused_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  ASSERT_LT(Function->Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, Classifies32BitMachOInstructionSectionAsText) {
  REQUIRE_LLVM_TARGET("armv7-apple-ios");
  const auto Object =
      makeAssemblyObject(llvm::Triple("armv7-apple-ios"),
                         ".section __TEXT,__custom,regular,pure_instructions\n"
                         ".globl _function\n_function:\nbx lr\n");
  auto Graph = ObjectReader::read(Object, Target::ARM);
  ASSERT_TRUE(Graph);
  const auto Function = std::find_if(
      Graph->symbols().begin(), Graph->symbols().end(),
      [](const auto &Symbol) { return Symbol.Name == "_function"; });
  ASSERT_NE(Function, Graph->symbols().end());
  EXPECT_EQ(Graph->sections()[Function->Section].Kind, SectionKind::Text);
}

TEST(ObjectReaderTest, ResolvesX86_64MachOSectionTargetRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".text\nnop\nLtarget:\nret\n.data\n.globl _pointer\n_pointer:\n.quad "
      "Ltarget\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  const auto &TargetSymbol = Graph->symbols()[Relocation.Symbol];
  ASSERT_LT(TargetSymbol.Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[TargetSymbol.Section].Kind, SectionKind::Text);
  EXPECT_EQ(Relocation.Addend, 1);
  EXPECT_FALSE(Relocation.AddendIsImplicit);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  auto Value = Internal::readUnsigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, Graph->sections()[TargetSymbol.Section].Address +
                        TargetSymbol.Offset + 1);
}

TEST(ObjectReaderTest, NormalizesX86_64MachOAbsoluteSectionRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".data\n.globl _pointer\n_pointer:\n.quad Lstring\n"
      ".section __TEXT,__cstring,cstring_literals\n.space 7\nLstring:\n"
      ".asciz \"value\"\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  ASSERT_LT(Symbol.Section, Graph->sections().size());
  EXPECT_EQ(Graph->sections()[Symbol.Section].Name, "__cstring");
  EXPECT_EQ(Relocation.Addend, 7);
  EXPECT_FALSE(Relocation.AddendIsImplicit);

  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  auto Value = Internal::readUnsigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  EXPECT_EQ(*Value, Graph->sections()[Symbol.Section].Address + 7);
}

TEST(ObjectReaderTest, PreservesExternalX86_64MachOPCRelativeRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Object = makeAssemblyObject(
      llvm::Triple("x86_64-apple-macosx"),
      ".text\n.globl _entry\n_entry:\ncallq _external\nretq\n"
      ".section __TEXT,__other,regular,pure_instructions\n"
      ".globl _external\n_external:\nretq\n");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "external-relocation.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  bool SymbolTarget = false;
  for (const auto &Section : MachO->sections()) {
    for (const auto &Relocation : Section.relocations()) {
      const auto Symbol = Relocation.getSymbol();
      ASSERT_NE(Symbol, MachO->symbol_end());
      auto Name = Symbol->getName();
      ASSERT_TRUE(static_cast<bool>(Name));
      EXPECT_EQ(*Name, "_external");
      SymbolTarget = true;
    }
  }
  ASSERT_TRUE(SymbolTarget);
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), 1U);
  const auto &Relocation = Graph->relocations().front();
  ASSERT_LT(Relocation.Symbol, Graph->symbols().size());
  EXPECT_EQ(Graph->symbols()[Relocation.Symbol].Name, "_external");
  EXPECT_EQ(Relocation.Type, llvm::MachO::X86_64_RELOC_BRANCH);
  EXPECT_EQ(Relocation.Addend, 0);
  EXPECT_TRUE(Relocation.AddendIsImplicit);
  ASSERT_TRUE(layout(*Graph, 0x1000));
  ASSERT_TRUE(applyRelocations(*Graph));
  const auto &Symbol = Graph->symbols()[Relocation.Symbol];
  auto Value = Internal::readSigned(
      Graph->sections()[Relocation.Section].Content, Relocation.Offset,
      Relocation.PatchSize, Endianness::Little);
  ASSERT_TRUE(Value);
  const uint64_t PatchEnd = Graph->sections()[Relocation.Section].Address +
                            Relocation.Offset + Relocation.PatchSize;
  uint64_t Resolved = 0;
  if (*Value < 0) {
    const uint64_t Magnitude =
        static_cast<uint64_t>(-(*Value + 1)) + UINT64_C(1);
    ASSERT_LE(Magnitude, PatchEnd);
    Resolved = PatchEnd - Magnitude;
  } else {
    const uint64_t Displacement = static_cast<uint64_t>(*Value);
    ASSERT_LE(Displacement, UINT64_MAX - PatchEnd);
    Resolved = PatchEnd + Displacement;
  }
  EXPECT_EQ(Resolved,
            Graph->sections()[Symbol.Section].Address + Symbol.Offset);
}

TEST(ObjectReaderTest, NormalizesX86_64MachOSectionRelocationOffsets) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  constexpr std::string_view Assembly = R"(
.section __TEXT,__text,regular,pure_instructions
.globl _entry
_entry:
cmpb $0, Ldata(%rip)
cmpw $256, Ldata(%rip)
cmpl $65536, Ldata(%rip)
cmpq %rax, Ldata(%rip)
callq Ltarget
ret
.section __TEXT,__other,regular,pure_instructions
.space 9
Ltarget:
ret
.data
.space 13
Ldata:
.quad 0
)";
  const auto Bytes = makeAssemblyObject(llvm::Triple("x86_64-apple-macosx"),
                                        std::string(Assembly));
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "section-relocations.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);

  struct RawRelocation {
    uint64_t Offset;
    uint32_t Type;
    int64_t Value;
    uint64_t TargetOffset;
    std::string TargetSection;
  };
  std::vector<RawRelocation> RawRelocations;
  std::set<std::string> SymbolNames;
  for (const auto &Symbol : MachO->symbols()) {
    auto Name = Symbol.getName();
    ASSERT_TRUE(static_cast<bool>(Name));
    SymbolNames.insert(Name->str());
  }
  EXPECT_EQ(SymbolNames.count("Ldata"), 0U);
  EXPECT_EQ(SymbolNames.count("Ltarget"), 0U);

  std::set<uint32_t> Types;
  for (const auto &Section : MachO->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name != "__text")
      continue;
    auto Content = Section.getContents();
    ASSERT_TRUE(static_cast<bool>(Content));
    const std::vector<WasmEdge::Byte> SectionBytes(Content->bytes_begin(),
                                                   Content->bytes_end());
    for (const auto &Relocation : Section.relocations()) {
      const auto Type = static_cast<uint32_t>(Relocation.getType());
      ASSERT_TRUE(Type == llvm::MachO::X86_64_RELOC_SIGNED ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_1 ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ||
                  Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ||
                  Type == llvm::MachO::X86_64_RELOC_BRANCH);
      EXPECT_EQ(Relocation.getSymbol(), MachO->symbol_end());
      const auto TargetSection =
          MachO->getRelocationSection(Relocation.getRawDataRefImpl());
      ASSERT_NE(TargetSection, MachO->section_end());
      auto TargetName = sectionName(*TargetSection);
      ASSERT_TRUE(static_cast<bool>(TargetName));
      const int64_t Suffix = Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                             : Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                          : 0;
      auto Value = Internal::readSigned(SectionBytes, Relocation.getOffset(), 4,
                                        Endianness::Little);
      ASSERT_TRUE(Value);
      const int64_t TargetOffset =
          *Value + static_cast<int64_t>(Section.getAddress()) +
          static_cast<int64_t>(Relocation.getOffset()) + 4 + Suffix -
          static_cast<int64_t>(TargetSection->getAddress());
      const uint64_t ExpectedOffset =
          Type == llvm::MachO::X86_64_RELOC_BRANCH ? 9 : 13;
      EXPECT_EQ(TargetOffset, static_cast<int64_t>(ExpectedOffset));
      RawRelocations.push_back({Relocation.getOffset(), Type, *Value,
                                ExpectedOffset, TargetName->str()});
      Types.insert(Type);
    }
  }
  ASSERT_EQ(RawRelocations.size(), 5U);
  EXPECT_EQ(std::count_if(RawRelocations.begin(), RawRelocations.end(),
                          [](const auto &Value) {
                            return Value.Type ==
                                   llvm::MachO::X86_64_RELOC_BRANCH;
                          }),
            1);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_1), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_2), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_SIGNED_4), 0U);
  EXPECT_NE(Types.count(llvm::MachO::X86_64_RELOC_BRANCH), 0U);

  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->relocations().size(), RawRelocations.size());
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    SCOPED_TRACE(testing::Message()
                 << "type=" << Raw.Type << " offset=" << Raw.Offset
                 << " raw=" << Raw.Value << " target=" << Raw.TargetSection
                 << "+" << Raw.TargetOffset);
    EXPECT_FALSE(Relocation->AddendIsImplicit);
    EXPECT_EQ(Relocation->Addend, static_cast<int64_t>(Raw.TargetOffset));
    ASSERT_LT(Relocation->Symbol, Graph->symbols().size());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    EXPECT_EQ(Symbol.Offset, 0U);
    ASSERT_LT(Symbol.Section, Graph->sections().size());
    EXPECT_EQ(Graph->sections()[Symbol.Section].Name, Raw.TargetSection);
  }

  ASSERT_TRUE(layout(*Graph, 0x1000, 0x1000));
  const auto OrderedDelta = [](uint64_t TargetAddress, uint64_t PatchAddress) {
    return TargetAddress >= PatchAddress
               ? std::pair{true, TargetAddress - PatchAddress}
               : std::pair{false, PatchAddress - TargetAddress};
  };
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    const auto &PatchSection = Graph->sections()[Relocation->Section];
    const auto &TargetSection = Graph->sections()[Symbol.Section];
    EXPECT_NE(
        OrderedDelta(TargetSection.Address, PatchSection.Address),
        OrderedDelta(TargetSection.InputAddress, PatchSection.InputAddress));
  }
  ASSERT_TRUE(applyRelocations(*Graph));
  for (const auto &Raw : RawRelocations) {
    const auto Relocation = std::find_if(
        Graph->relocations().begin(), Graph->relocations().end(),
        [&](const auto &Value) {
          return Graph->sections()[Value.Section].Name == "__text" &&
                 Value.Offset == Raw.Offset && Value.Type == Raw.Type;
        });
    ASSERT_NE(Relocation, Graph->relocations().end());
    const auto &Symbol = Graph->symbols()[Relocation->Symbol];
    const int64_t S =
        static_cast<int64_t>(Graph->sections()[Symbol.Section].Address);
    const int64_t P = static_cast<int64_t>(
        Graph->sections()[Relocation->Section].Address + Relocation->Offset);
    const int64_t Suffix = Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_1   ? 1
                           : Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_2 ? 2
                           : Raw.Type == llvm::MachO::X86_64_RELOC_SIGNED_4 ? 4
                                                                            : 0;
    auto Patched =
        Internal::readSigned(Graph->sections()[Relocation->Section].Content,
                             Relocation->Offset, 4, Endianness::Little);
    ASSERT_TRUE(Patched);
    EXPECT_EQ(*Patched, S + Relocation->Addend - P - 4 - Suffix);
    EXPECT_EQ(P + *Patched + 4 + Suffix,
              S + static_cast<int64_t>(Raw.TargetOffset));
  }
}

TEST(ObjectReaderTest, RejectsInvalidX86_64MachOSectionRelocationOffsets) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto Original = makeX86_64MachOSectionOffsetObject();
  const auto Relocation = x86_64MachOSectionRelocation(Original);
  ASSERT_NE(Relocation.Field, 0U);
  ASSERT_NE(Relocation.Relocation, 0U);
  ASSERT_GT(Relocation.TargetSize, Relocation.TargetOffset);
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message) {
    auto Result = ObjectReader::read(Bytes, Target::X86_64);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error().Message, Message);
  };
  const auto SetTargetOffset = [&](std::vector<WasmEdge::Byte> Bytes,
                                   int64_t Offset) {
    const int64_t Delta =
        Offset - static_cast<int64_t>(Relocation.TargetOffset);
    write32le(Bytes, Relocation.Field,
              static_cast<uint32_t>(Relocation.Raw + Delta));
    return Bytes;
  };

  Reject(SetTargetOffset(Original, -1),
         "x86_64 Mach-O section relocation target is negative");
  Reject(SetTargetOffset(Original, static_cast<int64_t>(Relocation.TargetSize)),
         "x86_64 Mach-O section relocation target is outside its section");
  Reject(SetTargetOffset(Original,
                         static_cast<int64_t>(Relocation.TargetSize + 1)),
         "x86_64 Mach-O section relocation target is outside its section");

  auto Overflow = Original;
  const size_t TextHeader = machOSectionHeaderOffset(Overflow, "__text");
  ASSERT_NE(TextHeader, 0U);
  write64le(Overflow, TextHeader + 32, UINT64_MAX);
  Reject(std::move(Overflow),
         "x86_64 Mach-O section relocation offset overflows");
}

TEST(ObjectReaderTest, RejectsUnsupportedX86_64MachOSectionNormalization) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Unsupported = makeX86_64MachOSectionOffsetObject();
  const auto Relocation = x86_64MachOSectionRelocation(Unsupported);
  ASSERT_NE(Relocation.Relocation, 0U);
  auto Word = read32le(Unsupported, Relocation.Relocation + 4);
  Word &= ~UINT32_C(0xF0000000);
  Word |= llvm::MachO::X86_64_RELOC_GOT << 28;
  write32le(Unsupported, Relocation.Relocation + 4, Word);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Unsupported.data()),
                          Unsupported.size()),
          "unsupported-section-relocation.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Parsed);
  ASSERT_NE(MachO, nullptr);
  const auto Section = MachO->section_begin();
  ASSERT_NE(Section, MachO->section_end());
  const auto InputRelocation = Section->relocation_begin();
  ASSERT_NE(InputRelocation, Section->relocation_end());
  EXPECT_EQ(InputRelocation->getType(), llvm::MachO::X86_64_RELOC_GOT);
  EXPECT_EQ(InputRelocation->getSymbol(), MachO->symbol_end());
  EXPECT_NE(MachO->getRelocationSection(InputRelocation->getRawDataRefImpl()),
            MachO->section_end());
  auto Result = ObjectReader::read(Unsupported, Target::X86_64);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "unsupported PC-relative x86_64 Mach-O section relocation type");
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
}

TEST(ObjectReaderTest, RejectsWrongDwarfCompactUnwindOffset) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true, true, false, false, true);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_NE(Offsets.Content, 0U);
  EXPECT_EQ(read32le(Bytes, Offsets.Content + 12) & UINT32_C(0x0F000000),
            UINT32_C(0x03000000));
  write32le(Bytes, Offsets.Content + 12, UINT32_C(0x03000001));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, LeavesX86_64MachOEHOnlyUnchanged) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto EHOnly =
      makeObject(llvm::Triple("x86_64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, true);
  auto Graph = ObjectReader::read(EHOnly, Target::X86_64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(Graph->compactUnwind().empty());
  EXPECT_TRUE(std::any_of(Graph->sections().begin(), Graph->sections().end(),
                          [](const Section &Value) {
                            return Value.Purpose == SectionPurpose::EHFrame;
                          }));
}

TEST(ObjectReaderTest, AppendsELFEHFrameTerminator) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object = makeX86_64ELFReaderFixture();
  ASSERT_EQ(Object.size(), 848U);
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "unwind.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  uint64_t InputSize = 0;
  for (const auto &Section : (*Parsed)->sections()) {
    auto Name = sectionName(Section);
    ASSERT_TRUE(static_cast<bool>(Name));
    if (*Name == ".eh_frame")
      InputSize = Section.getSize();
  }
  ASSERT_NE(InputSize, 0U);

  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph) << Graph.error().Message;
  const auto EH =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Value) {
                     return Value.Purpose == SectionPurpose::EHFrame;
                   });
  ASSERT_NE(EH, Graph->sections().end());
  EXPECT_EQ(EH->VirtualSize, InputSize + 4);
  ASSERT_EQ(EH->Content.size(), InputSize + 4);
  EXPECT_EQ(
      std::vector<WasmEdge::Byte>(EH->Content.end() - 4, EH->Content.end()),
      std::vector<WasmEdge::Byte>(4));
}

TEST(ObjectReaderTest, TreatsELFPDataNameAsOrdinaryData) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Object =
      makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                         ".section .pdata,\"a\",@progbits\n.byte 1, 2, 3, 4\n");
  auto Graph = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Graph);
  const auto PData =
      std::find_if(Graph->sections().begin(), Graph->sections().end(),
                   [](const auto &Value) { return Value.Name == ".pdata"; });
  ASSERT_NE(PData, Graph->sections().end());
  EXPECT_EQ(PData->Purpose, SectionPurpose::Default);
  EXPECT_EQ(PData->Kind, SectionKind::ReadOnly);
}

TEST(ObjectReaderTest, ParsesAssembledX86_64MachOCompactUnwind) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  constexpr std::string_view CompactUnwind = R"(
.section __LD,__compact_unwind,regular,debug
.p2align 3
.quad _f0
.long 1
.long 0x01000000
.quad 0
.quad 0
)";
  const auto Bytes =
      makeObject(llvm::Triple("x86_64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(CompactUnwind));
  auto Graph = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Graph);
  ASSERT_EQ(Graph->compactUnwind().size(), 1U);
  const auto &Record = Graph->compactUnwind()[0];
  EXPECT_EQ(Graph->symbols()[Record.Function].Name, "_f0");
  EXPECT_EQ(Record.Length, 1U);
  EXPECT_EQ(Record.Encoding, 0x01000000U);
  EXPECT_FALSE(Record.Personality);
  EXPECT_FALSE(Record.LSDA);
}

TEST(ObjectReaderTest, RejectsMalformedAArch64MachOCompactUnwind) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto MakeCompact = [] {
    return makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0",
                      {}, false, false, "generic", {}, true);
  };
  const auto Reject = [](std::vector<WasmEdge::Byte> Bytes,
                         std::string_view Message) {
    auto Result = ObjectReader::read(Bytes, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find(Message), std::string::npos)
        << Result.error().Message;
  };

  constexpr std::string_view BadSizeAssembly = R"(
.section __LD,__compact_unwind,regular,debug
.space 31
)";
  auto BadSize =
      makeObject(llvm::Triple("arm64-apple-macosx"), false, false, "f0", {},
                 false, false, "generic", {}, false, false, false, false, false,
                 false, std::string(BadSizeAssembly));
  Reject(std::move(BadSize), "compact unwind size is not a multiple of 32");

  auto MissingFunction = MakeCompact();
  auto Offsets = compactUnwindObjectOffsets(MissingFunction);
  const auto FunctionRelocation =
      compactUnwindRelocationFileOffset(MissingFunction, 0);
  ASSERT_NE(FunctionRelocation, 0U);
  write32le(MissingFunction, FunctionRelocation, 16);
  Reject(std::move(MissingFunction),
         "compact unwind record lacks function relocation");

  auto UnsupportedField = MakeCompact();
  Offsets = compactUnwindObjectOffsets(UnsupportedField);
  const auto UnsupportedRelocation =
      compactUnwindRelocationFileOffset(UnsupportedField, 0);
  ASSERT_NE(UnsupportedRelocation, 0U);
  write32le(UnsupportedField, UnsupportedRelocation, 8);
  Reject(std::move(UnsupportedField),
         "unsupported compact unwind relocation field");

  auto ZeroLength = MakeCompact();
  Offsets = compactUnwindObjectOffsets(ZeroLength);
  write32le(ZeroLength, Offsets.Content + 8, 0);
  Reject(std::move(ZeroLength),
         "compact unwind function length must be non-zero");

  auto BadTarget = MakeCompact();
  Offsets = compactUnwindObjectOffsets(BadTarget);
  write64le(BadTarget, Offsets.Content, UINT64_MAX);
  Reject(std::move(BadTarget), "compact unwind target is outside its section");

  for (const size_t Field : {16U, 24U}) {
    auto UnrelocatedTarget = MakeCompact();
    Offsets = compactUnwindObjectOffsets(UnrelocatedTarget);
    write64le(UnrelocatedTarget, Offsets.Content + Field, 1);
    Reject(std::move(UnrelocatedTarget),
           "compact unwind target lacks relocation");
  }
}

TEST(ObjectReaderTest, RejectsDuplicateAArch64MachOCompactUnwindRelocation) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  auto Bytes = makeObject(llvm::Triple("arm64-apple-macosx"), false, false,
                          "f0", {}, false, false, "generic", {}, true, false,
                          false, false, false, false, {}, false, true);
  const auto Offsets = compactUnwindObjectOffsets(Bytes);
  ASSERT_GE(Offsets.Count, 2U);
  const auto SecondFunctionRelocation =
      compactUnwindRelocationFileOffset(Bytes, 32);
  ASSERT_NE(SecondFunctionRelocation, 0U);
  write32le(Bytes, SecondFunctionRelocation, 0);
  auto Result = ObjectReader::read(Bytes, Target::AArch64);
  ASSERT_FALSE(Result);
  EXPECT_NE(
      Result.error().Message.find("duplicate compact unwind relocation field"),
      std::string::npos);
}

TEST(ObjectReaderTest, RejectsMalformedBytes) {
  const std::vector<WasmEdge::Byte> Bytes{0x01, 0x02, 0x03, 0x04};
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, NormalizesZeroSectionAlignment) {
  EXPECT_EQ(Internal::normalizeSectionAlignment(0), 1U);
  EXPECT_EQ(Internal::normalizeSectionAlignment(16), 16U);
}

TEST(ObjectReaderTest, ParsesCOFFExportDirectives) {
  auto Exports = Internal::parseCOFFExports(
      " /DEFAULTLIB:libcmt /EXPORT:f0 /EXPORT:data,DATA "
      "/EXPORT:ordinal,NONAME /EXPORT:alias=real "
      "/EXPORT:\"wasm.code\",DATA /EXPORT:\"public.name\"=\"real.name\"");
  ASSERT_TRUE(Exports);
  EXPECT_EQ(Exports->at("alias"), "real");
  EXPECT_EQ(Exports->at("data"), "data");
  EXPECT_EQ(Exports->at("f0"), "f0");
  EXPECT_EQ(Exports->at("ordinal"), "ordinal");
  EXPECT_EQ(Exports->at("public.name"), "real.name");
  EXPECT_EQ(Exports->at("wasm.code"), "wasm.code");
}

TEST(ObjectReaderTest, ParsesGNUCOFFExportDirectives) {
  auto Exports = Internal::parseCOFFExports(
      " -export:f0 -export:\"wasm.code\",data /export:version");
  ASSERT_TRUE(Exports);
  EXPECT_EQ(Exports->size(), 3U);
  EXPECT_EQ(Exports->at("f0"), "f0");
  EXPECT_EQ(Exports->at("wasm.code"), "wasm.code");
  EXPECT_EQ(Exports->at("version"), "version");
}

TEST(ObjectReaderTest, RejectsMalformedCOFFExportDirectives) {
  EXPECT_FALSE(Internal::parseCOFFExports("/EXPORT:"));
  EXPECT_FALSE(Internal::parseCOFFExports("\"/EXPORT:f0"));
}

TEST(ObjectReaderTest, AcceptsNullSymbolRISCVRelaxRelocation) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
#if LLVM_VERSION_MAJOR < 11
  GTEST_SKIP() << "the RISC-V assembler does not accept relocation names in "
                  ".reloc before LLVM 11";
#endif
  const auto Object =
      makeAssemblyObject(llvm::Triple("riscv64-unknown-linux-gnu"),
                         ".text\n.reloc 0, R_RISCV_RELAX, 0\nret\n");
  auto Parsed =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Object.data()),
                          Object.size()),
          "relax.o"));
  ASSERT_TRUE(static_cast<bool>(Parsed));
  bool HasNullRelax = false;
  for (const auto &Section : (*Parsed)->sections())
    for (const auto &Relocation : Section.relocations())
      HasNullRelax |= Relocation.getType() == llvm::ELF::R_RISCV_RELAX &&
                      Relocation.getSymbol() == (*Parsed)->symbol_end();
  ASSERT_TRUE(HasNullRelax);

  auto Graph = ObjectReader::read(Object, Target::RISCV64);
  ASSERT_TRUE(Graph);
  EXPECT_TRUE(std::none_of(Graph->relocations().begin(),
                           Graph->relocations().end(), [](const auto &Value) {
                             return Value.Type == llvm::ELF::R_RISCV_RELAX;
                           }));
}

TEST(ObjectReaderTest, NormalizesX86_64ELFRelaObject) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  const auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  auto Result = ObjectReader::read(Bytes, Target::X86_64);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->target(), Target::X86_64);
  EXPECT_EQ(Result->endianness(), Endianness::Little);
  EXPECT_FALSE(Result->sections().empty());
  const auto Text =
      std::find_if(Result->sections().begin(), Result->sections().end(),
                   [](const Section &Value) { return Value.Name == ".text"; });
  ASSERT_NE(Text, Result->sections().end());
  EXPECT_EQ(Text->Kind, SectionKind::Text);
  EXPECT_EQ(Text->VirtualSize, Text->Content.size());
  EXPECT_GT(Text->Alignment, 0U);
  const auto Bss = std::find_if(
      Result->sections().begin(), Result->sections().end(),
      [](const Section &Value) { return Value.Kind == SectionKind::BSS; });
  ASSERT_NE(Bss, Result->sections().end());
  EXPECT_TRUE(Bss->Content.empty());
  EXPECT_GE(Bss->VirtualSize, 4U);
  EXPECT_GE(Bss->Alignment, 16U);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  const auto TextId = static_cast<SectionId>(Text - Result->sections().begin());
  const auto F0Id = static_cast<SymbolId>(F0 - Result->symbols().begin());
  EXPECT_EQ(F0->Section, TextId);
  EXPECT_EQ(F0->Offset, 0U);
  EXPECT_TRUE(F0->Exported);
  EXPECT_GT(F0->Size, 0U);
  ASSERT_FALSE(Result->relocations().empty());
  const auto &Relocation = Result->relocations()[0];
  EXPECT_EQ(Relocation.Section, TextId);
  EXPECT_LT(Relocation.Offset, Text->Content.size());
  EXPECT_NE(Relocation.Type, 0U);
  EXPECT_LT(Relocation.Symbol, Result->symbols().size());
  EXPECT_EQ(Relocation.Offset, 3U);
  EXPECT_EQ(Relocation.Type, 42U);
  EXPECT_EQ(Relocation.Format, ObjectFormat::ELF);
  EXPECT_EQ(Relocation.PatchSize, 4U);
  EXPECT_EQ(Result->symbols()[Relocation.Symbol].Name, "value");
  EXPECT_EQ(Relocation.Addend, -4);
  EXPECT_FALSE(Relocation.AddendIsImplicit);
  EXPECT_NE(Relocation.Symbol, F0Id);
}

TEST(ObjectReaderTest, ReadsCOFFExportsFromDirectives) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, true),
      Target::X86_64);
  ASSERT_TRUE(Result);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  EXPECT_TRUE(F0->Exported);
}

TEST(ObjectReaderTest, NormalizesRenamedCOFFExport) {
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Result =
      ObjectReader::read(makeObject(llvm::Triple("x86_64-pc-windows-msvc"),
                                    false, false, "real", "/EXPORT:alias=real"),
                         Target::X86_64);
  ASSERT_TRUE(Result);
  const auto Real =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "real"; });
  ASSERT_NE(Real, Result->symbols().end());
  EXPECT_TRUE(Real->Exported);
  ASSERT_TRUE(Real->ExportName);
  EXPECT_EQ(*Real->ExportName, "alias");
}

TEST(ObjectReaderTest, KeepsMachOVersionMinCommandKind) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Legacy = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-apple-macosx10.13")), Target::X86_64);
  ASSERT_TRUE(Legacy);
  ASSERT_EQ(Legacy->machOBuildVersions().size(), 1U);
  EXPECT_TRUE(Legacy->machOBuildVersions()[0].VersionMin);
  EXPECT_EQ(Legacy->machOBuildVersions()[0].MinimumOS, UINT32_C(0x000A0D00));

  auto Current = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-apple-macosx10.15")), Target::X86_64);
  ASSERT_TRUE(Current);
  ASSERT_EQ(Current->machOBuildVersions().size(), 1U);
  EXPECT_FALSE(Current->machOBuildVersions()[0].VersionMin);
}

TEST(ObjectReaderTest, ReadsWindowsGNUExportDirectives) {
  REQUIRE_LLVM_TARGET("x86_64-w64-windows-gnu");
  const auto Object =
      makeObject(llvm::Triple("x86_64-w64-windows-gnu"), false, true);
  const std::string_view Text(reinterpret_cast<const char *>(Object.data()),
                              Object.size());
  ASSERT_NE(Text.find("-export:"), std::string_view::npos);
  auto Result = ObjectReader::read(Object, Target::X86_64);
  ASSERT_TRUE(Result);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  EXPECT_TRUE(F0->Exported);
}

TEST(ObjectReaderTest, RejectsAmbiguousCOFFExportNames) {
  EXPECT_FALSE(Internal::parseCOFFExports("/EXPORT:alias=a /EXPORT:alias=b"));
  EXPECT_TRUE(Internal::parseCOFFExports("/EXPORT:alias=a /EXPORT:alias=a"));
  REQUIRE_LLVM_TARGET("x86_64-pc-windows-msvc");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-pc-windows-msvc"), false, false, "real",
                 "/EXPORT:first=real /EXPORT:second=real"),
      Target::X86_64);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "COFF symbol has more than one export name");
}

TEST(ObjectReaderTest, ExportsMachOExternalDefinedSymbols) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("x86_64-apple-macosx")), Target::X86_64);
  ASSERT_TRUE(Result);
  const auto F0 =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "_f0"; });
  ASSERT_NE(F0, Result->symbols().end());
  EXPECT_TRUE(F0->Exported);
  ASSERT_EQ(Result->relocations().size(), 1U);
  EXPECT_EQ(Result->relocations()[0].PatchSize, 4U);
}

TEST(ObjectReaderTest, RejectsArm64eMachOSubtypes) {
  constexpr size_t MachOCPUSubtypeOffset = 8;
  constexpr uint32_t CPU_SUBTYPE_ARM64_ALL = 0;
  constexpr uint32_t CPU_SUBTYPE_ARM64E = 2;
  constexpr uint32_t CPU_SUBTYPE_ARM64E_PTRAUTH_VERSION_1 = 0x81000002;
  constexpr uint32_t CPU_SUBTYPE_LIB64 = 0x80000000;
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  const auto Generic = makeObject(llvm::Triple("arm64-apple-macosx"));
  ASSERT_GE(Generic.size(), MachOCPUSubtypeOffset + sizeof(uint32_t));
  ASSERT_EQ(read32le(Generic, MachOCPUSubtypeOffset), CPU_SUBTYPE_ARM64_ALL);
  EXPECT_TRUE(ObjectReader::read(Generic, Target::AArch64));
  auto GenericWithCapabilities = Generic;
  ASSERT_TRUE(Internal::writeUnsigned(
      GenericWithCapabilities, MachOCPUSubtypeOffset, 4, Endianness::Little,
      CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_ARM64_ALL));
  EXPECT_TRUE(ObjectReader::read(GenericWithCapabilities, Target::AArch64));
  for (const uint32_t Subtype :
       {CPU_SUBTYPE_ARM64E, CPU_SUBTYPE_ARM64E_PTRAUTH_VERSION_1,
        CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_ARM64E}) {
    auto Arm64e = Generic;
    ASSERT_TRUE(Internal::writeUnsigned(Arm64e, MachOCPUSubtypeOffset, 4,
                                        Endianness::Little, Subtype));
    auto Result = ObjectReader::read(Arm64e, Target::AArch64);
    ASSERT_FALSE(Result);
    EXPECT_NE(Result.error().Message.find("including arm64e"),
              std::string::npos);
  }
}

TEST(ObjectReaderTest, AcceptsX86_64MachOSubtypeControl) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  const auto X86 = makeObject(llvm::Triple("x86_64-apple-macosx"));
  EXPECT_TRUE(ObjectReader::read(X86, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsMalformedX86_64MachOSignedMetadata) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Bytes = makeObject(llvm::Triple("x86_64-apple-macosx"));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "test.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() != Section.relocation_end()) {
      const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
      llvm::object::DataRefImpl SectionRef;
      SectionRef.d.a = Ref.d.a;
      RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
      break;
    }
  }
  ASSERT_NE(RelocationOffset, 0U);
  for (const uint32_t Mask : {UINT32_C(1) << 24, UINT32_C(1) << 25}) {
    auto Malformed = Bytes;
    write32le(Malformed, RelocationOffset + 4,
              read32le(Malformed, RelocationOffset + 4) ^ Mask);
    EXPECT_FALSE(ObjectReader::read(Malformed, Target::X86_64));
  }
  auto Scattered = Bytes;
  write32le(Scattered, RelocationOffset,
            read32le(Scattered, RelocationOffset) | (UINT32_C(1) << 31));
  EXPECT_FALSE(ObjectReader::read(Scattered, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsScatteredAArch64MachORelocation) {
  REQUIRE_LLVM_TARGET("arm64-apple-macosx");
  EXPECT_FALSE(
      Internal::supportsMachORelocationMetadata(Target::AArch64, true));
  auto Bytes = makeObject(llvm::Triple("arm64-apple-macosx"));
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "arm64.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  const auto *MachO = llvm::dyn_cast<llvm::object::MachOObjectFile>(&**Object);
  ASSERT_NE(MachO, nullptr);
  size_t RelocationOffset = 0;
  for (const auto &Section : MachO->sections()) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    const auto Ref = Section.relocation_begin()->getRawDataRefImpl();
    llvm::object::DataRefImpl SectionRef;
    SectionRef.d.a = Ref.d.a;
    RelocationOffset = MachO->getSection64(SectionRef).reloff + Ref.d.b * 8;
    break;
  }
  ASSERT_NE(RelocationOffset, 0U);
  write32le(Bytes, RelocationOffset,
            read32le(Bytes, RelocationOffset) | (UINT32_C(1) << 31));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::AArch64));
}

TEST(ObjectReaderTest, DoesNotExportHiddenMachOSymbols) {
  REQUIRE_LLVM_TARGET("x86_64-apple-macosx");
  auto Result =
      ObjectReader::read(makeObject(llvm::Triple("x86_64-apple-macosx"), false,
                                    false, "hidden", {}, true),
                         Target::X86_64);
  ASSERT_TRUE(Result);
  const auto Hidden =
      std::find_if(Result->symbols().begin(), Result->symbols().end(),
                   [](const Symbol &Value) { return Value.Name == "_hidden"; });
  ASSERT_NE(Hidden, Result->symbols().end());
  EXPECT_FALSE(Hidden->Exported);
}

TEST(ObjectReaderTest, RejectsELFRelocationWithInvalidSectionLink) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  Bytes[Header + 40] = 0xFF;
  Bytes[Header + 41] = 0xFF;
  Bytes[Header + 42] = 0xFF;
  Bytes[Header + 43] = 0x7F;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsELFRelocationWithZeroEntrySize) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 56, 0);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsRelocationFreeELFWithZeroSymbolEntrySize) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeAssemblyObject(llvm::Triple("x86_64-unknown-linux-gnu"),
                                  R"(
.text
.globl leaf
leaf:
  ret
)");
  ASSERT_FALSE(Bytes.empty());
  const auto Header = elf64SymbolTableSectionHeader(Bytes);
  ASSERT_NE(Header, 0U);
  write64le(Bytes, Header + 56, 0);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsTruncatedELFRelocationTable) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 32, read64le(Bytes, Header + 32) - 1);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsELFRelocationWithInvalidSymbolIndex) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  write64le(Bytes, RelocationOffset + 8, UINT64_C(0xFFFFFFFF00000000));
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, PreservesGraphRelocationDiagnosticFields) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  write64le(Bytes, RelocationOffset, UINT64_MAX);

  auto Result = ObjectReader::read(Bytes, Target::X86_64);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message,
            "relocation offset is outside section content");
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_TRUE(Result.error().RelocationType);
  EXPECT_EQ(Result.error().Offset, UINT64_MAX);
}

TEST(ObjectReaderTest, ClassifiesUnsupportedELFRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  const auto Header = elf64RelocationSectionHeader(Bytes);
  const auto RelocationOffset = read64le(Bytes, Header + 24);
  const auto Info = read64le(Bytes, RelocationOffset + 8);
  write64le(Bytes, RelocationOffset + 8,
            (Info & UINT64_C(0xFFFFFFFF00000000)) | UINT32_MAX);

  auto Result = ObjectReader::read(Bytes, Target::X86_64);

  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "unsupported relocation patch size");
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
  EXPECT_TRUE(Result.error().Section);
  EXPECT_TRUE(Result.error().Symbol);
  EXPECT_EQ(Result.error().RelocationType, UINT32_MAX);
}

#if LLVM_VERSION_MAJOR < 19
#else
TEST(ObjectReaderTest, NormalizesELFCrelRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Result = ObjectReader::read(makeX86_64CrelObject(), Target::X86_64);
  ASSERT_TRUE(Result);
  ASSERT_EQ(Result->relocations().size(), 1U);
  EXPECT_EQ(Result->relocations()[0].Offset, 3U);
  EXPECT_EQ(Result->relocations()[0].Type, 42U);
  EXPECT_EQ(Result->relocations()[0].Addend, -4);
  EXPECT_FALSE(Result->relocations()[0].AddendIsImplicit);
}

TEST(ObjectReaderTest, RejectsTruncatedELFCrelRelocation) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeX86_64CrelObject();
  const auto Header = elf64RelocationSectionHeader(Bytes);
  write64le(Bytes, Header + 32, 1);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}
#endif

TEST(ObjectReaderTest, MarksELFRelAddendsImplicit) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Result = ObjectReader::read(
      makeObject(llvm::Triple("armv7-unknown-linux-gnueabihf"), false, false,
                 "f0", {}, false, true),
      Target::ARM);
  ASSERT_TRUE(Result);
  ASSERT_FALSE(Result->relocations().empty());
  EXPECT_TRUE(Result->relocations()[0].AddendIsImplicit);
  EXPECT_EQ(Result->relocations()[0].Addend, 0);
}

TEST(ObjectReaderTest, ReadsARMVFPArgumentAttributes) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  constexpr std::array<std::pair<unsigned, uint32_t>, 4> Cases{{
      {llvm::ARMBuildAttrs::BaseAAPCS, llvm::ELF::EF_ARM_ABI_FLOAT_SOFT},
      {llvm::ARMBuildAttrs::HardFPAAPCS, llvm::ELF::EF_ARM_ABI_FLOAT_HARD},
      {llvm::ARMBuildAttrs::CompatibleFPAAPCS,
       llvm::ELF::EF_ARM_ABI_FLOAT_SOFT},
      {llvm::ARMBuildAttrs::ToolChainFPPCS, 0},
  }};
  for (const auto &[Attribute, FloatFlags] : Cases) {
    auto Bytes = makeARMAttributeObject(Attribute);
    ASSERT_FALSE(armAttributeSections(Bytes).empty());
    auto Result = ObjectReader::read(Bytes, Target::ARM);
    ASSERT_TRUE(Result) << Attribute << ": "
                        << (Result ? "" : Result.error().Message);
    EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_EABIMASK,
              llvm::ELF::EF_ARM_EABI_VER5)
        << Attribute;
    EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                    llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
              FloatFlags)
        << Attribute;
    EXPECT_TRUE(std::none_of(Result->sections().begin(),
                             Result->sections().end(), [](const auto &Section) {
                               return Section.Name.find("attributes") !=
                                      std::string::npos;
                             }));
  }
}

TEST(ObjectReaderTest, TreatsARMSoftFPAsBaseAAPCS) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Result = ObjectReader::read(
      makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS,
                             ".text\n vadd.f32 s0, s0, s0\n", "+vfp3"),
      Target::ARM);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_ABI_FLOAT_SOFT,
            llvm::ELF::EF_ARM_ABI_FLOAT_SOFT);
  EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_ABI_FLOAT_HARD, 0U);
}

TEST(ObjectReaderTest, DefaultsMissingARMVFPArgumentAttributeToSoft) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Result =
      ObjectReader::read(makeARMAttributeObject(std::nullopt), Target::ARM);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Result->elfFlags() & llvm::ELF::EF_ARM_ABI_FLOAT_SOFT,
            llvm::ELF::EF_ARM_ABI_FLOAT_SOFT);
}

TEST(ObjectReaderTest, RejectsUnsupportedARMHeaderFlags) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS);
  constexpr uint32_t Unsupported = UINT32_C(0x00800000);
  write32le(Bytes, 36,
            llvm::ELF::EF_ARM_EABI_VER4 | Unsupported |
                llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error().Message, "unsupported ARM ELF header flags");
  EXPECT_EQ(Result.error().Kind, DiagnosticKind::Unsupported);
}

TEST(ObjectReaderTest, WritesAttributeNormalizedARMFlags) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS);
  write32le(Bytes, 36,
            llvm::ELF::EF_ARM_EABI_VER4 | llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result);
  constexpr uint32_t Expected =
      llvm::ELF::EF_ARM_EABI_VER5 | llvm::ELF::EF_ARM_ABI_FLOAT_SOFT;
  ASSERT_EQ(Result->elfFlags(), Expected);
  ASSERT_TRUE(ELFWriter::layout(*Result));
  ASSERT_TRUE(applyRelocations(*Result));
  std::vector<WasmEdge::Byte> OutputBytes;
  Writer Output(OutputBytes);
  ASSERT_TRUE(ELFWriter::write(*Result, Output));
  ASSERT_GE(OutputBytes.size(), 40U);
  EXPECT_EQ(read32le(OutputBytes, 36), Expected);
}

TEST(ObjectReaderTest, RejectsMalformedARMAttributes) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::HardFPAAPCS);
  const auto Sections = armAttributeSections(Bytes);
  ASSERT_FALSE(Sections.empty());
  ASSERT_NE(Sections[0].second, 0U);
  Bytes[Sections[0].first] = 'B';
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, RejectsUnknownARMVFPArgumentAttribute) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  EXPECT_FALSE(ObjectReader::read(makeARMAttributeObject(4), Target::ARM));
}

TEST(ObjectReaderTest, RejectsConflictingARMAttributeSections) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS,
                                      R"(
.section .ARM.attributes.extra,"",%progbits
.byte 0x41
.long 0x11
.asciz "aeabi"
.byte 0x01
.long 0x07
.byte 0x1c, 0x01
)");
  markARMAttributeSection(Bytes, ".ARM.attributes.extra");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, RejectsConflictingARMAttributesWithinSection) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS,
                                      R"(
.section .ARM.attributes.duplicates,"",%progbits
.byte 0x41
.long 0x13
.asciz "aeabi"
.byte 0x01
.long 0x09
.byte 0x1c, 0x00, 0x1c, 0x01
)");
  markARMAttributeSection(Bytes, ".ARM.attributes.duplicates");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, ReadsSectionScopedARMVFPArgumentAttribute) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_section_attributes,"",%progbits
.byte 0x41
.long 0x13
.asciz "aeabi"
.byte 0x02
.long 0x09
.byte 0x01, 0x00
.byte 0x1c, 0x01
)");
  markARMAttributeSection(Bytes, "arm_section_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result) << (Result ? "" : Result.error().Message);
  EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
            llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
}

TEST(ObjectReaderTest, ReadsSymbolScopedARMVFPArgumentAttribute) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_symbol_attributes,"",%progbits
.byte 0x41
.long 0x13
.asciz "aeabi"
.byte 0x03
.long 0x09
.byte 0x01, 0x00
.byte 0x1c, 0x02
)");
  markARMAttributeSection(Bytes, "arm_symbol_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result) << (Result ? "" : Result.error().Message);
  EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
            0U);
}

TEST(ObjectReaderTest, RejectsConflictingARMAttributeScopes) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::BaseAAPCS, R"(
.section arm_scoped_attributes,"",%progbits
.byte 0x41
.long 0x1c
.asciz "aeabi"
.byte 0x02
.long 0x09
.byte 0x01, 0x00
.byte 0x1c, 0x01
.byte 0x03
.long 0x09
.byte 0x01, 0x00
.byte 0x1c, 0x02
)");
  markARMAttributeSection(Bytes, "arm_scoped_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, RejectsARMScopedAttributeCrossingSubsectionBoundary) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_scoped_boundary,"",%progbits
.byte 0x41
.long 0x1a
.asciz "aeabi"
.byte 0x02
.long 0x06
.byte 0x01, 0x00
.byte 0x01
.long 0x07
.byte 0x1c, 0x01
)");
  markARMAttributeSection(Bytes, "arm_scoped_boundary");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, IgnoresPrivateARMAttributeVendor) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_private_attributes,"",%progbits
.byte 0x41
.long 0x0f
.asciz "private"
.byte 0xff, 0x80, 0x7f
.long 0x11
.asciz "aeabi"
.byte 0x01
.long 0x07
.byte 0x1c, 0x01
)");
  markARMAttributeSection(Bytes, "arm_private_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  const auto Original = Bytes;
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result) << (Result ? "" : Result.error().Message);
  EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
            llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
  EXPECT_EQ(Bytes, Original);
}

TEST(ObjectReaderTest, IgnoresMixedCasePrivateARMAttributeVendors) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  for (const std::string Vendor : {"AEABI", "Aeabi"}) {
    auto Bytes = makeARMAttributeObject(
        std::nullopt, ".section arm_mixed_case_attributes,\"\",%progbits\n"
                      ".byte 0x41\n"
                      ".long 0x0d\n"
                      ".asciz \"" +
                          Vendor +
                          "\"\n"
                          ".byte 0xff, 0x80, 0x7f\n"
                          ".long 0x11\n"
                          ".asciz \"aeabi\"\n"
                          ".byte 0x01\n"
                          ".long 0x07\n"
                          ".byte 0x1c, 0x01\n");
    markARMAttributeSection(Bytes, "arm_mixed_case_attributes");
    EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
    const auto Original = Bytes;
    auto Result = ObjectReader::read(Bytes, Target::ARM);
    ASSERT_TRUE(Result) << Vendor << ": "
                        << (Result ? "" : Result.error().Message);
    EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                    llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
              llvm::ELF::EF_ARM_ABI_FLOAT_HARD)
        << Vendor;
    EXPECT_EQ(Bytes, Original) << Vendor;
  }
}

TEST(ObjectReaderTest, IgnoresManyPrivateARMAttributeVendors) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  std::string Assembly =
      ".section arm_many_private_attributes,\"\",%progbits\n.byte 0x41\n";
  for (unsigned I = 0; I < 256; ++I)
    Assembly += ".long 0x0a\n.asciz \"p" + std::to_string(I % 10) +
                "\"\n.byte 0xde, 0xad, 0xbe\n";
  Assembly += R"(
.long 0x11
.asciz "aeabi"
.byte 0x01
.long 0x07
.byte 0x1c, 0x01
)";
  auto Bytes = makeARMAttributeObject(std::nullopt, std::move(Assembly));
  markARMAttributeSection(Bytes, "arm_many_private_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  const auto Original = Bytes;
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result) << (Result ? "" : Result.error().Message);
  EXPECT_EQ(Result->elfFlags() & (llvm::ELF::EF_ARM_ABI_FLOAT_SOFT |
                                  llvm::ELF::EF_ARM_ABI_FLOAT_HARD),
            llvm::ELF::EF_ARM_ABI_FLOAT_HARD);
  EXPECT_EQ(Bytes, Original);
}

TEST(ObjectReaderTest, RejectsUnterminatedPrivateARMAttributeVendor) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_private_attributes,"",%progbits
.byte 0x41
.long 0x07
.ascii "abc"
)");
  markARMAttributeSection(Bytes, "arm_private_attributes");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, RejectsTrailingByteInARMAttributes) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(std::nullopt, R"(
.section arm_trailing_attribute_byte,"",%progbits
.byte 0x41
.long 0x11
.asciz "aeabi"
.byte 0x01
.long 0x07
.byte 0x1c, 0x01
.byte 0x00
)");
  markARMAttributeSection(Bytes, "arm_trailing_attribute_byte");
  EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM));
}

TEST(ObjectReaderTest, RejectsTrailingBytesInARMAttributes) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  for (const std::string Tail :
       {".byte 0x00, 0x00, 0x00\n", ".byte 0x00, 0x01, 0x00\n"}) {
    auto Bytes = makeARMAttributeObject(
        std::nullopt, ".section arm_trailing_attribute_bytes,\"\",%progbits\n"
                      ".byte 0x41\n"
                      ".long 0x11\n"
                      ".asciz \"aeabi\"\n"
                      ".byte 0x01\n"
                      ".long 0x07\n"
                      ".byte 0x1c, 0x01\n" +
                          Tail);
    markARMAttributeSection(Bytes, "arm_trailing_attribute_bytes");
    EXPECT_EQ(armAttributeSections(Bytes).size(), 2U);
    EXPECT_FALSE(ObjectReader::read(Bytes, Target::ARM)) << Tail;
  }
}

TEST(ObjectReaderTest, OmitsAllocatableARMAttributeSections) {
  REQUIRE_LLVM_TARGET("armv7-unknown-linux-gnueabihf");
  auto Bytes = makeARMAttributeObject(llvm::ARMBuildAttrs::HardFPAAPCS);
  markARMAttributeSection(Bytes, ".ARM.attributes", llvm::ELF::SHF_ALLOC);
  auto Object =
      llvm::object::ObjectFile::createObjectFile(llvm::MemoryBufferRef(
          llvm::StringRef(reinterpret_cast<const char *>(Bytes.data()),
                          Bytes.size()),
          "allocatable-arm-attributes.o"));
  ASSERT_TRUE(static_cast<bool>(Object));
  bool FoundAllocatableAttributes = false;
  for (const auto &Section : (*Object)->sections()) {
    const llvm::object::ELFSectionRef ELFSection(Section);
    FoundAllocatableAttributes |=
        ELFSection.getType() == llvm::ELF::SHT_ARM_ATTRIBUTES &&
        (ELFSection.getFlags() & llvm::ELF::SHF_ALLOC) != 0;
  }
  ASSERT_TRUE(FoundAllocatableAttributes);
  auto Result = ObjectReader::read(Bytes, Target::ARM);
  ASSERT_TRUE(Result) << (Result ? "" : Result.error().Message);
  EXPECT_TRUE(std::none_of(
      Result->sections().begin(), Result->sections().end(),
      [](const auto &Section) { return Section.Name == ".ARM.attributes"; }));
}

TEST(ObjectReaderTest, PreservesRISCVArchitectureFlags) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto Bytes = makeAssemblyObject(
      llvm::Triple("riscv64-unknown-linux-gnu"),
      ".option rvc\n.text\n.globl f0\nf0:\n c.nop\n ret\n", "+c,+f,+d");
  auto Graph = ObjectReader::read(Bytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  const uint32_t Expected =
      llvm::ELF::EF_RISCV_RVC | llvm::ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
  EXPECT_EQ(Graph->elfFlags(), Expected);
}

TEST(ObjectReaderTest, WritesRISCVArchitectureFlags) {
  REQUIRE_LLVM_TARGET("riscv64-unknown-linux-gnu");
  const auto Bytes = makeAssemblyObject(
      llvm::Triple("riscv64-unknown-linux-gnu"),
      ".option rvc\n.text\n.globl f0\nf0:\n c.nop\n ret\n", "+c,+f,+d");
  auto Graph = ObjectReader::read(Bytes, Target::RISCV64);
  ASSERT_TRUE(Graph);
  const uint32_t Expected =
      llvm::ELF::EF_RISCV_RVC | llvm::ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
  ASSERT_TRUE(ELFWriter::layout(*Graph));
  ASSERT_TRUE(applyRelocations(*Graph));
  std::vector<WasmEdge::Byte> OutputBytes;
  Writer Output(OutputBytes);
  ASSERT_TRUE(ELFWriter::write(*Graph, Output));
  EXPECT_EQ(read64le(OutputBytes, 48) & UINT32_MAX, Expected);
}

TEST(ObjectReaderTest, RejectsNonRelocatableELFObject) {
  REQUIRE_LLVM_TARGET("x86_64-unknown-linux-gnu");
  auto Bytes = makeObject(llvm::Triple("x86_64-unknown-linux-gnu"));
  ASSERT_GT(Bytes.size(), 18U);
  Bytes[16] = 3;
  Bytes[17] = 0;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
  Bytes[16] = 2;
  EXPECT_FALSE(ObjectReader::read(Bytes, Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUnsupportedObjectFormat) {
  REQUIRE_LLVM_TARGET("wasm32-unknown-unknown");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("wasm32-unknown-unknown")), Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUnsupportedArchitecture) {
  REQUIRE_LLVM_TARGET("i386-unknown-linux-gnu");
  EXPECT_FALSE(ObjectReader::read(
      makeObject(llvm::Triple("i386-unknown-linux-gnu")), Target::X86_64));
}

TEST(ObjectReaderTest, RejectsUndefinedExternal) {
  EXPECT_FALSE(ObjectReader::read(makeNativeObject(true), nativeTarget()));
}

TEST(ObjectReaderTest, RejectsTargetMismatch) {
  const Target Other =
      nativeTarget() == Target::X86_64 ? Target::AArch64 : Target::X86_64;
  EXPECT_FALSE(ObjectReader::read(makeNativeObject(), Other));
}

TEST(ObjectReaderTest, RejectsEmptyAndArchiveBuffers) {
  EXPECT_FALSE(ObjectReader::read({}, nativeTarget()));
  const std::string Archive = "!<arch>\n";
  EXPECT_FALSE(ObjectReader::read(
      WasmEdge::Span<const WasmEdge::Byte>(
          reinterpret_cast<const WasmEdge::Byte *>(Archive.data()),
          Archive.size()),
      nativeTarget()));
}

TEST(SemanticSymbolTest, AcceptsOnlyCanonicalIndexedNames) {
  using WasmEdge::AOT::semanticSymbolIndex;
  EXPECT_EQ(semanticSymbolIndex("f0", 'f'), UINT64_C(0));
  EXPECT_EQ(semanticSymbolIndex("f12", 'f'), UINT64_C(12));
  EXPECT_EQ(semanticSymbolIndex("t3", 't'), UINT64_C(3));
  for (const std::string_view Name :
       {"f", "f01", "f-1", "f+1", "f1x", "floor", "fmin", "fmax", "t", "trunc",
        "t00", "f18446744073709551616"}) {
    SCOPED_TRACE(Name);
    EXPECT_FALSE(semanticSymbolIndex(Name, Name.front()));
  }
  EXPECT_FALSE(semanticSymbolIndex("t3", 'f'));
}

} // namespace
