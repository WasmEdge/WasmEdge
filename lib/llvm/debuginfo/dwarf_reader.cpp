// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "debuginfo/dwarf_reader.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/DebugInfo/DWARF/DWARFCompileUnit.h>
#include <llvm/Support/Error.h>

#include <fmt/format.h>

#include <algorithm>
#include <unordered_map>

namespace WasmEdge::LLVM::DebugInfo {

namespace {

bool readULEB(Span<const Byte> &Data, uint64_t &Value) noexcept {
  Value = 0;
  for (uint32_t Shift = 0; Shift < 64; Shift += 7) {
    if (Data.empty()) {
      return false;
    }
    const auto B = static_cast<uint8_t>(Data[0]);
    Data = Data.subspan(1);
    Value |= static_cast<uint64_t>(B & 0x7FU) << Shift;
    if ((B & 0x80U) == 0) {
      return true;
    }
  }
  return false;
}

} // namespace

std::unique_ptr<DwarfReader>
DwarfReader::create(const AST::Module &Mod) noexcept {
  std::unique_ptr<DwarfReader> R(new DwarfReader());
  for (const auto &Custom : Mod.getCustomSections()) {
    std::string_view Name = Custom.getName();
    if (Name.substr(0, 7) != ".debug_") {
      continue;
    }
    const auto Content = Custom.getContent();
    R->Sections[llvm::StringRef(Name.data() + 1, Name.size() - 1)] =
        llvm::MemoryBuffer::getMemBuffer(
            llvm::StringRef(reinterpret_cast<const char *>(Content.data()),
                            Content.size()),
            llvm::StringRef(Name.data(), Name.size()), false);
  }
  if (!R->Sections.count("debug_info") || !R->Sections.count("debug_line")) {
    return nullptr;
  }
  auto Errors = R->Errors;
  auto Handler = [Errors](llvm::Error E) {
    ++*Errors;
    llvm::consumeError(std::move(E));
  };
#if LLVM_VERSION_MAJOR < 11
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true);
#else
  R->Ctx = llvm::DWARFContext::create(R->Sections, 4, true, Handler, Handler);
#endif
  if (!R->Ctx) {
    return nullptr;
  }
  uint64_t UnitsEnd = 0;
  for (const auto &Unit : R->Ctx->info_section_units()) {
    UnitsEnd = std::max<uint64_t>(UnitsEnd, Unit->getNextUnitOffset());
  }
  if (UnitsEnd != R->Sections["debug_info"]->getBufferSize()) {
    ++*Errors;
  }
  R->CodeOffset = Mod.getCodeSection().getContentOffset();
  R->indexSubprograms(Mod);
  R->parseNames(Mod);
  return R;
}

void DwarfReader::indexSubprograms(const AST::Module &Mod) noexcept {
  std::unordered_map<uint64_t, llvm::DWARFDie> ByLowPC;
  for (const auto &CU : Ctx->compile_units()) {
    for (const auto &Entry : CU->dies()) {
      llvm::DWARFDie Die(CU.get(), &Entry);
      if (Die.getTag() != llvm::dwarf::DW_TAG_subprogram) {
        continue;
      }
      if (auto LowPC =
              llvm::dwarf::toAddress(Die.find(llvm::dwarf::DW_AT_low_pc))) {
        if (*LowPC != 0 && *LowPC != UINT32_C(0xFFFFFFFF)) {
          ByLowPC.emplace(*LowPC, Die);
        }
      }
    }
  }
  const auto Segs = Mod.getCodeSection().getContent();
  Subprograms.resize(Segs.size());
  for (size_t I = 0; I < Segs.size(); ++I) {
    for (uint64_t Off : {Segs[I].getSegOffset(), Segs[I].getBodyOffset()}) {
      if (auto It = ByLowPC.find(toAddress(Off)); It != ByLowPC.end()) {
        Subprograms[I] = It->second;
        break;
      }
    }
  }
}

void DwarfReader::parseNames(const AST::Module &Mod) noexcept {
  for (const auto &Custom : Mod.getCustomSections()) {
    if (Custom.getName() != "name") {
      continue;
    }
    Span<const Byte> Data = Custom.getContent();
    while (!Data.empty()) {
      const auto Id = static_cast<uint8_t>(Data[0]);
      Data = Data.subspan(1);
      uint64_t Size = 0;
      if (!readULEB(Data, Size) || Size > Data.size()) {
        return;
      }
      auto Sub = Data.first(Size);
      Data = Data.subspan(Size);
      if (Id != 1) {
        continue;
      }
      uint64_t Count = 0;
      if (!readULEB(Sub, Count)) {
        return;
      }
      for (uint64_t I = 0; I < Count; ++I) {
        uint64_t Index = 0, Len = 0;
        if (!readULEB(Sub, Index) || !readULEB(Sub, Len) || Len > Sub.size()) {
          return;
        }
        Names.emplace(
            static_cast<uint32_t>(Index),
            std::string(reinterpret_cast<const char *>(Sub.data()), Len));
        Sub = Sub.subspan(Len);
      }
    }
  }
}

llvm::DWARFDie
DwarfReader::getSubprogram(uint32_t DefinedIndex) const noexcept {
  if (DefinedIndex >= Subprograms.size()) {
    return {};
  }
  return Subprograms[DefinedIndex];
}

std::string DwarfReader::getFunctionName(uint32_t FuncIndex) const {
  if (auto It = Names.find(FuncIndex); It != Names.end()) {
    return It->second;
  }
  return fmt::format("func[{}]", FuncIndex);
}

} // namespace WasmEdge::LLVM::DebugInfo
